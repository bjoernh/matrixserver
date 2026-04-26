#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/trivial.hpp>
#include <chrono>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <random>
#include <signal.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#include "Server.h"

// Extract the binary path from the env var (first whitespace-separated token).
// The rest (shell redirections, "&") are handled by fork/exec directly.
static std::string defaultAppPath([]() -> std::string {
  const char *envVal = std::getenv("MATRIXSERVER_DEFAULT_APP");
  std::string cmd = envVal ? std::string(envVal)
                           : std::string("/usr/local/bin/MainMenu");
  // Trim to first token in case the env var still contains shell arguments
  auto pos = cmd.find_first_of(" \t");
  return (pos != std::string::npos) ? cmd.substr(0, pos) : cmd;
}());
bool defaultAppStarted = false;

Server::~Server() {
  // Stop the io_context so ioThread exits cleanly.
  ioWork.reset(); // drop work guard so run() can return
  ioContext.stop();
  if (ioThread && ioThread->joinable()) {
    ioThread->join();
  }
}

std::shared_ptr<App> Server::getAppByID(int searchID) {
  std::lock_guard<std::mutex> lock(appsMutex);
  for (auto &app : apps) {
    if (app->getAppId() == searchID) {
      return app;
    }
  }
  return nullptr;
}

Server::Server(std::shared_ptr<IRenderer> setRenderer,
               matrixserver::ServerConfig &setServerConfig)
    : ioContext(),
      ioWork(new boost::asio::executor_work_guard<
             boost::asio::io_context::executor_type>(
          boost::asio::make_work_guard(ioContext))),
      serverConfig(setServerConfig),
      tcpServer(
          ioContext,
          boost::asio::ip::tcp::endpoint(
              boost::asio::ip::tcp::v4(),
              std::stoi(setServerConfig.serverconnection().serverport()))),
      //        unixServer(ioContext,
      //        boost::asio::local::stream_protocol::endpoint("/tmp/matrixserver.sock")),
      ipcServer("matrixserver"), joystickmngr(8) {
  boost::log::core::get()->set_filter(boost::log::trivial::severity >=
                                      boost::log::trivial::debug);

  setRenderer->setClientMessageCallback(
      [this, setRenderer](std::shared_ptr<matrixserver::MatrixServerMessage> msg) {
        if (msg->messagetype() == matrixserver::getServerInfo) {
          auto response = std::make_shared<matrixserver::MatrixServerMessage>();
          response->set_messagetype(matrixserver::getServerInfo);
          auto *tempConfig = new matrixserver::ServerConfig();
          tempConfig->CopyFrom(serverConfig);
          response->set_allocated_serverconfig(tempConfig);
          setRenderer->sendMessage(response);

          std::lock_guard<std::mutex> lock(appsMutex);
          if (!apps.empty()) {
            auto schemaMsg = std::make_shared<matrixserver::MatrixServerMessage>();
            schemaMsg->set_appid(apps.back()->getAppId());
            schemaMsg->set_messagetype(matrixserver::appParamSchema);
            auto schema = apps.back()->getParamSchema();
            schemaMsg->mutable_appparamschema()->CopyFrom(schema);
            setRenderer->sendMessage(schemaMsg);
          }
        } else if (msg->messagetype() == matrixserver::imuData ||
            msg->messagetype() == matrixserver::audioDataMessage ||
            msg->messagetype() == matrixserver::joystickData ||
            msg->messagetype() == matrixserver::setAppParam ||
            msg->messagetype() == matrixserver::getAppParams) {
          std::lock_guard<std::mutex> lock(appsMutex);
          if (msg->messagetype() == matrixserver::setAppParam ||
              msg->messagetype() == matrixserver::getAppParams) {
            // If appID is specified, target that app, else target current top app
            if (msg->appid() != 0) {
              for (auto &app : apps) {
                if (app->getAppId() == msg->appid()) {
                  app->getConnection()->sendMessage(msg);
                  break;
                }
              }
            } else if (!apps.empty()) {
              apps.back()->getConnection()->sendMessage(msg);
            }
          } else {
            for (auto &app : apps) {
              app->getConnection()->sendMessage(msg);
            }
          }
        }
      });

  renderers.push_back(setRenderer);
  tcpServer.setAcceptCallback(
      std::bind(&Server::newConnectionCallback, this, std::placeholders::_1));
  //    unixServer.setAcceptCallback(std::bind(&Server::newConnectionCallback,
  //    this, std::placeholders::_1));
  ipcServer.setAcceptCallback(
      std::bind(&Server::newConnectionCallback, this, std::placeholders::_1));
  ioThread = std::make_unique<std::thread>([this]() {
    try {
      this->ioContext.run();
    } catch (const std::exception &e) {
      BOOST_LOG_TRIVIAL(fatal) << "[Server] io_service exception: " << e.what();
    }
  });
  std::random_device rd;
  srand(rd());
}

void Server::newConnectionCallback(
    std::shared_ptr<UniversalConnection> connection) {
  BOOST_LOG_TRIVIAL(debug) << "[matrixserver] NEW SocketConnection CALLBACK!";
  connection->setReceiveCallback(std::bind(&Server::handleRequest, this,
                                           std::placeholders::_1,
                                           std::placeholders::_2));
  connections.push_back(connection);
}

void Server::handleRequest(
    std::shared_ptr<UniversalConnection> connection,
    std::shared_ptr<matrixserver::MatrixServerMessage> message) {
  switch (message->messagetype()) {
  case matrixserver::registerApp:
    if (message->appid() == 0) {
      BOOST_LOG_TRIVIAL(debug)
          << "[matrixserver] register new App request received";
      int newAppId;
      {
        std::lock_guard<std::mutex> lock(appsMutex);
        apps.push_back(std::make_shared<App>(connection));
        newAppId = apps.back()->getAppId();
      }
      auto response = std::make_shared<matrixserver::MatrixServerMessage>();
      response->set_appid(newAppId);
      response->set_messagetype(matrixserver::registerApp);
      response->set_status(matrixserver::success);
      connection->sendMessage(response);
    }
    break;
  case matrixserver::getServerInfo: {
    BOOST_LOG_TRIVIAL(debug)
        << "[matrixserver] get ServerInfo request received";
    auto response = std::make_shared<matrixserver::MatrixServerMessage>();
    response->set_messagetype(matrixserver::getServerInfo);
    auto *tempServerConfig = new matrixserver::ServerConfig();
    tempServerConfig->CopyFrom(serverConfig);
    response->set_allocated_serverconfig(tempServerConfig);
    connection->sendMessage(response);
    break;
  }
  case matrixserver::requestScreenAccess:
    // TODO App level logic, set App on top, pause all other Apps, which aren't
    // on top any more
    break;
  case matrixserver::setScreenFrame: {
    bool isTopApp = false;
    {
      std::lock_guard<std::mutex> lock(appsMutex);
      if (!apps.empty() && message->appid() == apps.back()->getAppId()) {
        isTopApp = true;
      }
    }
    if (isTopApp) {
      for (const auto& renderer : renderers) {
        for (const auto& screenInfo : message->screendata()) {
          int sid = screenInfo.screenid();
          if (sid < 0 || sid >= static_cast<int>(serverConfig.screeninfo_size())) {
            BOOST_LOG_TRIVIAL(warning) << "[Server] Invalid screen ID: " << sid;
            continue;
          }
          renderer->setScreenData(sid,
                                  (Color *)screenInfo.framedata()
                                      .data());
        }
        auto usStart = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch());
        //                  std::thread([renderer](){renderer->render();}).detach();
        renderer->render();
        auto response = std::make_shared<matrixserver::MatrixServerMessage>();
        response->set_messagetype(matrixserver::setScreenFrame);
        response->set_status(matrixserver::success);
        connection->sendMessage(response);
        auto usTotal =
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::system_clock::now().time_since_epoch()) -
            usStart;
        if (message->has_serverconfig())
          renderer->setGlobalBrightness(
              message->serverconfig().globalscreenbrightness());
        // BOOST_LOG_TRIVIAL(debug) << "[Server] rendertime: " <<
        // usTotal.count() << " us"; // ~ 15ms
      }
    } else {
      // send app to pause
      BOOST_LOG_TRIVIAL(debug)
          << "[Server] send app " << message->appid() << " to pause";
      auto msg = std::make_shared<matrixserver::MatrixServerMessage>();
      msg->set_messagetype(matrixserver::appKill);
      connection->sendMessage(msg);
    }
    break;
  }
  case matrixserver::appAlive:
  case matrixserver::appPause:
  case matrixserver::appResume:
  case matrixserver::appKill: {
    BOOST_LOG_TRIVIAL(debug)
        << "[Server] appkill " << message->appid() << " successfull";
    std::lock_guard<std::mutex> lock(appsMutex);
    apps.erase(std::remove_if(apps.begin(), apps.end(),
                              [message](const std::shared_ptr<App>& a) {
                                if (a->getAppId() == message->appid()) {
                                  BOOST_LOG_TRIVIAL(debug)
                                      << "[Server] App " << message->appid()
                                      << " deleted";
                                  return true;
                                } else {
                                  return false;
                                }
                              }),
               apps.end());
    break;
  }
  case matrixserver::appParamSchema: {
    BOOST_LOG_TRIVIAL(debug) << "[Server] App parameter schema received";
    {
      std::lock_guard<std::mutex> lock(appsMutex);
      for (auto &app : apps) {
        if (app->getAppId() == message->appid()) {
          app->setParamSchema(message->appparamschema());
          break;
        }
      }
    }
    // Forward to all renderers
    for (const auto& renderer : renderers) {
      renderer->sendMessage(message);
    }
    break;
  }
  case matrixserver::appParamValues: {
    // Forward current values to all renderers
    for (const auto& renderer : renderers) {
      renderer->sendMessage(message);
    }
    break;
  }
  default:
    break;
  }
}

bool Server::tick() {
  if (joystickmngr.getButtonPress(7)) {
    std::lock_guard<std::mutex> lock(appsMutex);
    if (apps.size() > 0) {
      BOOST_LOG_TRIVIAL(debug) << "kill current app" << std::endl;
      auto msg = std::make_shared<matrixserver::MatrixServerMessage>();
      msg->set_messagetype(matrixserver::appKill);
      apps.back()->sendMsg(msg);
    }
  }
  joystickmngr.clearAllButtonPresses();

  {
    std::lock_guard<std::mutex> lock(appsMutex);
    if (apps.size() == 0 && !defaultAppStarted) {
      BOOST_LOG_TRIVIAL(info) << "[Server] Starting default app: " << defaultAppPath;
      pid_t pid = fork();
      if (pid == 0) {
        // Child: redirect stdout/stderr to /dev/null and exec the app
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull >= 0) {
          dup2(devnull, STDOUT_FILENO);
          dup2(devnull, STDERR_FILENO);
          close(devnull);
        }
        setsid(); // detach from server's process group
        execl(defaultAppPath.c_str(), defaultAppPath.c_str(), nullptr);
        _exit(1); // execl failed
      } else if (pid > 0) {
        defaultAppPid_ = pid;
        BOOST_LOG_TRIVIAL(debug) << "[Server] Default app PID: " << pid;
      } else {
        BOOST_LOG_TRIVIAL(warning) << "[Server] fork() failed for default app";
      }
      defaultAppStarted = true;
    }
    if (apps.size() > 0) {
      defaultAppStarted = false;
      // Reap default app if it exited on its own (e.g. another app took over)
      if (defaultAppPid_ > 0 && waitpid(defaultAppPid_, nullptr, WNOHANG) != 0)
        defaultAppPid_ = -1;
    }

    apps.erase(std::remove_if(apps.begin(), apps.end(),
                              [](const std::shared_ptr<App>& a) {
                                bool returnVal = a->getConnection()->isDead();
                                if (returnVal)
                                  BOOST_LOG_TRIVIAL(debug)
                                      << "[matrixserver] App " << a->getAppId()
                                      << " deleted";
                                return returnVal;
                              }),
               apps.end());
  }

  connections.erase(
      std::remove_if(connections.begin(), connections.end(),
                     [](std::shared_ptr<UniversalConnection> con) {
                       bool returnVal = con->isDead();
                       if (returnVal) {
                         BOOST_LOG_TRIVIAL(debug)
                             << "[matrixserver] Connection deleted";
                       }
                       return returnVal;
                     }),
      connections.end());
  return true;
}

void Server::stopDefaultApp() {
  if (defaultAppPid_ > 0) {
    BOOST_LOG_TRIVIAL(info) << "[Server] Stopping default app (PID " << defaultAppPid_ << ")";
    kill(defaultAppPid_, SIGTERM);
    // Wait briefly for graceful exit, then force-kill
    for (int i = 0; i < 30; ++i) {
      usleep(100'000);
      if (waitpid(defaultAppPid_, nullptr, WNOHANG) != 0) {
        defaultAppPid_ = -1;
        return;
      }
    }
    BOOST_LOG_TRIVIAL(warning) << "[Server] Default app did not exit, sending SIGKILL";
    kill(defaultAppPid_, SIGKILL);
    waitpid(defaultAppPid_, nullptr, 0);
    defaultAppPid_ = -1;
  }
}

void Server::addRenderer(std::shared_ptr<IRenderer> newRenderer) {
  newRenderer->setClientMessageCallback(
      [this, newRenderer](std::shared_ptr<matrixserver::MatrixServerMessage> msg) {
        if (msg->messagetype() == matrixserver::getServerInfo) {
          auto response = std::make_shared<matrixserver::MatrixServerMessage>();
          response->set_messagetype(matrixserver::getServerInfo);
          auto *cfg = new matrixserver::ServerConfig();
          cfg->CopyFrom(serverConfig);
          response->set_allocated_serverconfig(cfg);
          newRenderer->sendMessage(response);

          std::lock_guard<std::mutex> lock(appsMutex);
          if (!apps.empty()) {
            auto schemaMsg = std::make_shared<matrixserver::MatrixServerMessage>();
            schemaMsg->set_appid(apps.back()->getAppId());
            schemaMsg->set_messagetype(matrixserver::appParamSchema);
            schemaMsg->mutable_appparamschema()->CopyFrom(apps.back()->getParamSchema());
            newRenderer->sendMessage(schemaMsg);
          }
        } else if (msg->messagetype() == matrixserver::imuData ||
            msg->messagetype() == matrixserver::audioDataMessage ||
            msg->messagetype() == matrixserver::joystickData ||
            msg->messagetype() == matrixserver::setAppParam ||
            msg->messagetype() == matrixserver::getAppParams) {
          std::lock_guard<std::mutex> lock(appsMutex);
          if (msg->messagetype() == matrixserver::setAppParam ||
              msg->messagetype() == matrixserver::getAppParams) {
            if (msg->appid() != 0) {
              for (auto &app : apps) {
                if (app->getAppId() == msg->appid()) {
                  app->getConnection()->sendMessage(msg);
                  break;
                }
              }
            } else if (!apps.empty()) {
              apps.back()->getConnection()->sendMessage(msg);
            }
          } else {
            for (auto &app : apps) {
              app->getConnection()->sendMessage(msg);
            }
          }
        }
      });
  renderers.push_back(newRenderer);
}
