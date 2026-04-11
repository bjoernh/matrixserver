#include "MatrixApplication.h"
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/trivial.hpp>
#include <random>
#include <chrono>
#include <thread>
#ifndef _WIN32
#include <sys/time.h>
#endif

bool updateBrightness = false;

float MatrixApplication::latestSimulatorImuX = 0.0f;
float MatrixApplication::latestSimulatorImuY = 0.0f;
float MatrixApplication::latestSimulatorImuZ = 0.0f;
std::mutex MatrixApplication::simulatorImuMutex;

uint8_t MatrixApplication::latestAudioVolume = 0;
std::vector<uint8_t> MatrixApplication::latestAudioFrequencies;
std::mutex MatrixApplication::audioDataMutex;

MatrixApplication::MatrixApplication(int fps, std::string serverUri, std::string appName)
    : mainThread(), io_context(), serverUri(serverUri), appName(appName) {
  boost::log::core::get()->set_filter(boost::log::trivial::severity >=
                                      boost::log::trivial::debug);
  std::random_device rd;
  srand(rd());
  setFps(fps);
  appId = 0;
  appState = AppState::starting;
  //    ioThread = new boost::thread([this]() { io_context.run(); });
  while (!connect(serverUri)) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  registerAtServer();
}

bool MatrixApplication::connect(const std::string &server_uri) {
  BOOST_LOG_TRIVIAL(debug) << "[Application] Trying to connect to Server: "
                           << server_uri;

  // Parse URI scheme
  auto schemeEnd = server_uri.find("://");
  if (schemeEnd == std::string::npos) {
    BOOST_LOG_TRIVIAL(error)
        << "[Application] Invalid URI format: " << server_uri;
    return false;
  }

  std::string scheme = server_uri.substr(0, schemeEnd);
  std::string rest = server_uri.substr(schemeEnd + 3);

  if (scheme == "ipc") {
    // ipc://<path>
    auto ipcCon = std::make_shared<IpcConnection>();
    ipcCon->connectToServer(rest);
    connection = ipcCon;
  } else if (scheme == "tcp") {
    // tcp://<host>:<port>
    auto colonPos = rest.rfind(':');
    if (colonPos == std::string::npos) {
      BOOST_LOG_TRIVIAL(error)
          << "[Application] Invalid TCP URI, expected tcp://<host>:<port>: "
          << server_uri;
      return false;
    }
    std::string host = rest.substr(0, colonPos);
    std::string port = rest.substr(colonPos + 1);
    connection = TcpClient::connect(io_context, host, port);
  } else if (scheme == "unix") {
#ifndef _WIN32
    // unix:///tmp/<path>.socket
    // After "unix://", rest includes the full path (e.g.
    // "/tmp/matrixserver.sock")
    connection = UnixSocketClient::connect(io_context, rest);
#else
    BOOST_LOG_TRIVIAL(error) << "[Application] Unix sockets not supported on Windows";
    return false;
#endif
  } else {
    BOOST_LOG_TRIVIAL(error) << "[Application] Unknown URI scheme: " << scheme;
    return false;
  }

  if (!connection->isDead()) {
    BOOST_LOG_TRIVIAL(debug) << "[Application] Connection successfull";
    ioThread = new boost::thread([this]() { io_context.run(); });
    connection->setReceiveCallback(bind(&MatrixApplication::handleRequest, this,
                                        std::placeholders::_1,
                                        std::placeholders::_2));
    return true;
  } else {
    BOOST_LOG_TRIVIAL(debug) << "[Application] Connection failed";
    return false;
  }
}

void MatrixApplication::registerAtServer() {
  BOOST_LOG_TRIVIAL(trace) << "[Application] try to register at server";
  auto message = std::make_shared<matrixserver::MatrixServerMessage>();
  message->set_messagetype(matrixserver::registerApp);
  connection->sendMessage(message);
}

void MatrixApplication::renderToScreens() {
  auto startTime = micros();
  auto setScreenMessage = std::make_shared<matrixserver::MatrixServerMessage>();
  setScreenMessage->set_messagetype(matrixserver::setScreenFrame);
  setScreenMessage->set_appid(appId);
  int i = 0;
  for (auto screen : screens) {
    auto screenData = setScreenMessage->add_screendata();
    screenData->set_screenid(screen->getScreenId());
    screenData->set_framedata((char *)screen->getScreenData().data(),
                              screen->getScreenDataSize() * sizeof(Color));
    screenData->set_encoding(matrixserver::ScreenData_Encoding_rgb24bbp);
  }
  //    std::cout << "data ready: " << micros() - startTime << "us" <<
  //    std::endl;
  if (updateBrightness) {
    auto *tempServerConfig = new matrixserver::ServerConfig();
    tempServerConfig->CopyFrom(serverConfig);
    setScreenMessage->set_allocated_serverconfig(tempServerConfig);
    updateBrightness = false;
  }

  connection->sendMessage(setScreenMessage);
  //    std::cout << "data sent:  " << micros() - startTime << "us" <<
  //    std::endl;
}

void MatrixApplication::internalLoop() {
  bool running = true;
  while (running) {
    auto startTime = micros();
    if (appState == AppState::running) {
      renderSyncMutex.lock();
      running = loop();
      renderToScreens();
    }
    if (appState == AppState::killed) {
      running = false;
    }
    checkConnection();
    auto sleepTime = (1000000 / fps) - (micros() - startTime);
    if (sleepTime > 0) {
      std::this_thread::sleep_for(std::chrono::microseconds(sleepTime));
    } else {
      //            BOOST_LOG_TRIVIAL(warning) << "[Application] FPS drop, load:
      //            " << load;
    }
    load = 1.0f - ((float)sleepTime / (1000000.0f / (float)fps));
    //        BOOST_LOG_TRIVIAL(warning) << "[Application] rendertime: " <<
    //        micros()-startTime << " us";
  }
}

void MatrixApplication::checkConnection() {
  if (connection->isDead()) {
    appState = AppState::failure;
    if (connect(serverUri))
      registerAtServer();
  }
}

void MatrixApplication::handleRequest(
    std::shared_ptr<UniversalConnection> connection,
    std::shared_ptr<matrixserver::MatrixServerMessage> message) {
  BOOST_LOG_TRIVIAL(trace) << "[Application] handleRequest called";
  switch (message->messagetype()) {
  case matrixserver::registerApp:
    if (message->status() == matrixserver::success) {
      BOOST_LOG_TRIVIAL(debug)
          << "[Application] Register at Server successfull";
      appId = message->appid();

      // Send parameter schema if any are registered
      auto schemaMsg = std::make_shared<matrixserver::MatrixServerMessage>();
      schemaMsg->set_messagetype(matrixserver::appParamSchema);
      schemaMsg->set_appid(appId);
      auto schema = params.toSchema(appName);
      schemaMsg->mutable_appparamschema()->CopyFrom(schema);
      connection->sendMessage(schemaMsg);

      auto response = std::make_shared<matrixserver::MatrixServerMessage>();
      response->set_messagetype(matrixserver::getServerInfo);
      response->set_appid(appId);
      connection->sendMessage(response);
    }
    break;
  case matrixserver::getServerInfo:
    BOOST_LOG_TRIVIAL(debug)
        << "[Application] ServerInfo received, setup complete!";
    serverConfig.Clear();
    serverConfig.CopyFrom(message->serverconfig());
    for (auto screenInfo : serverConfig.screeninfo()) {
      screens.push_back(std::make_shared<Screen>(
          screenInfo.width(), screenInfo.height(), screenInfo.screenid()));
    }
    appState = AppState::running;
    break;
  case matrixserver::appPause: {
    auto response = std::make_shared<matrixserver::MatrixServerMessage>();
    response->set_messagetype(matrixserver::appPause);
    response->set_appid(appId);
    if (pause()) {
      response->set_status(matrixserver::success);
      BOOST_LOG_TRIVIAL(debug) << "app Paused";
    } else
      response->set_status(matrixserver::error);
    connection->sendMessage(response);
  } break;
  case matrixserver::appAlive: {
    auto response = std::make_shared<matrixserver::MatrixServerMessage>();
    response->set_messagetype(matrixserver::appAlive);
    response->set_appid(appId);
    if (appState == AppState::running || appState == AppState::paused)
      response->set_status(matrixserver::success);
    else
      response->set_status(matrixserver::error);
    connection->sendMessage(response);
  } break;
  case matrixserver::appResume: {
    auto response = std::make_shared<matrixserver::MatrixServerMessage>();
    response->set_messagetype(matrixserver::appResume);
    response->set_appid(appId);
    if (resume())
      response->set_status(matrixserver::success);
    else
      response->set_status(matrixserver::error);
    connection->sendMessage(response);
  } break;
  case matrixserver::appKill: {
    auto response = std::make_shared<matrixserver::MatrixServerMessage>();
    response->set_messagetype(matrixserver::appKill);
    response->set_appid(appId);
    response->set_status(matrixserver::success);
    connection->sendMessage(response);
    BOOST_LOG_TRIVIAL(debug) << "app killed";
    stop();
  } break;
  case matrixserver::requestScreenAccess:
  case matrixserver::setScreenFrame:
    renderSyncMutex.unlock();
    break;
  case matrixserver::imuData: {
    std::lock_guard<std::mutex> lock(MatrixApplication::simulatorImuMutex);
    MatrixApplication::latestSimulatorImuX = message->imudata().accelx();
    MatrixApplication::latestSimulatorImuY = message->imudata().accely();
    MatrixApplication::latestSimulatorImuZ = message->imudata().accelz();
  } break;
  case matrixserver::audioDataMessage: {
    if (message->has_audiodata()) {
      std::lock_guard<std::mutex> lock(MatrixApplication::audioDataMutex);
      MatrixApplication::latestAudioVolume =
          static_cast<uint8_t>(message->audiodata().volume());
      MatrixApplication::latestAudioFrequencies.clear();
      for (int i = 0; i < message->audiodata().frequencybands_size(); ++i) {
        MatrixApplication::latestAudioFrequencies.push_back(
            static_cast<uint8_t>(message->audiodata().frequencybands(i)));
      }
    }
  } break;
  case matrixserver::setAppParam:
    if (message->has_appparamupdate()) {
      params.applyUpdate(message->appparamupdate());
    }
    break;
  case matrixserver::getAppParams: {
    auto response = std::make_shared<matrixserver::MatrixServerMessage>();
    response->set_messagetype(matrixserver::appParamValues);
    response->set_status(matrixserver::success);
    response->set_appid(appId);
    auto values = params.toValues(appName);
    response->mutable_appparamvalues()->CopyFrom(values);
    connection->sendMessage(response);
    break;
  }
  default:
    break;
  }
}

int MatrixApplication::getFps() { return fps; }

void MatrixApplication::setFps(int setFps) {
  if (setFps <= MAXFPS && setFps >= MINFPS) {
    fps = setFps;
  } else if (setFps == 0) {
    fps = DEFAULTFPS;
  }
}

AppState MatrixApplication::getAppState() { return appState; }

float MatrixApplication::getLoad() { return load; }

void MatrixApplication::start() {
  mainThread = new boost::thread(&MatrixApplication::internalLoop, this);
}

bool MatrixApplication::pause() {
  if (appState == AppState::running) {
    appState = AppState::paused;
    return true;
  }
  return false;
}

bool MatrixApplication::resume() {
  if (appState == AppState::paused) {
    appState = AppState::running;
    return true;
  }
  return false;
}

void MatrixApplication::stop() {
  exit(0); // exit immediately because the below thread join won't work all the
           // time TODO:fix this!
  //    if (mainThread != NULL) {
  //        appState = AppState::killed;
  //        mainThread->interrupt();
  //        mainThread->join();
  //        mainThread = NULL;
  //    }
  //    BOOST_LOG_TRIVIAL(debug)  << "app stop successfull";
  //    exit(0);
}

int MatrixApplication::getBrightness() {
  return serverConfig.globalscreenbrightness();
}

void MatrixApplication::setBrightness(int setBrightness) {
  serverConfig.set_globalscreenbrightness(setBrightness);
  updateBrightness = true;
}

long MatrixApplication::micros() {
#ifdef _WIN32
  return std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::high_resolution_clock::now().time_since_epoch()).count();
#else
  struct timeval tp;
  gettimeofday(&tp, nullptr);
  long us = tp.tv_sec * 1000000 + tp.tv_usec;
  return us;
#endif
}
