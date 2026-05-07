#include "MatrixApplication.h"
#include "ConnectionFactory.h"
#include <Joystick.h>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/trivial.hpp>
#include <random>
#include <sys/time.h>

bool updateBrightness = false;

float MatrixApplication::latestSimulatorImuX = 0.0f;
float MatrixApplication::latestSimulatorImuY = 0.0f;
float MatrixApplication::latestSimulatorImuZ = 0.0f;
float MatrixApplication::latestSimulatorGyroX = 0.0f;
float MatrixApplication::latestSimulatorGyroY = 0.0f;
float MatrixApplication::latestSimulatorGyroZ = 0.0f;
std::mutex MatrixApplication::simulatorImuMutex;

uint8_t MatrixApplication::latestAudioVolume = 0;
std::vector<uint8_t> MatrixApplication::latestAudioFrequencies;
std::mutex MatrixApplication::audioDataMutex;

MatrixApplication::MatrixApplication(int fps, std::string serverUri, std::string appName) : io_context(), serverUri(serverUri), appName(appName) {
    boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::debug);
    std::random_device rd;
    srand(rd());
    setFps(fps);
    appId = 0;
    runner_.setState(AppState::starting);
    setupDispatcher();
    while (!connect(serverUri)) {
        sleep(1);
    }
}

// ---------------------------------------------------------------------------
// Connection
// ---------------------------------------------------------------------------

bool MatrixApplication::connect(const std::string& server_uri) {
    BOOST_LOG_TRIVIAL(debug) << "[Application] Trying to connect to Server: " << server_uri;

    connection = ConnectionFactory::connectFromUri(io_context, server_uri);

    if (!connection || connection->isDead()) {
        BOOST_LOG_TRIVIAL(debug) << "[Application] Connection failed";
        return false;
    }

    BOOST_LOG_TRIVIAL(debug) << "[Application] Connection successfull";
    // Reset io_context before restarting it (needed on reconnect)
    io_context.restart();
    ioThread = std::make_unique<std::thread>([this]() { io_context.run(); });
    connection->setReceiveCallback(bind(&MatrixApplication::handleRequest, this, std::placeholders::_1, std::placeholders::_2));
    return true;
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
    for (const auto& screen : screens) {
        auto screenData = setScreenMessage->add_screendata();
        screenData->set_screenid(screen->getScreenId());
        screenData->set_framedata((char*)screen->getScreenData().data(), screen->getScreenDataSize() * sizeof(Color));
        screenData->set_encoding(matrixserver::ScreenData_Encoding_rgb24bbp);
    }
    //    std::cout << "data ready: " << micros() - startTime << "us" <<
    //    std::endl;
    if (updateBrightness) {
        auto* tempServerConfig = new matrixserver::ServerConfig();
        tempServerConfig->CopyFrom(serverConfig);
        setScreenMessage->set_allocated_serverconfig(tempServerConfig);
        updateBrightness = false;
    }

    connection->sendMessage(setScreenMessage);
    //    std::cout << "data sent:  " << micros() - startTime << "us" <<
    //    std::endl;
}

void MatrixApplication::checkConnection() {
    if (connection->isDead()) {
        runner_.setState(AppState::failure);
        if (connect(serverUri))
            registerAtServer();
    }
}

// ---------------------------------------------------------------------------
// Message dispatcher setup
// ---------------------------------------------------------------------------

void MatrixApplication::setupDispatcher() {
    using Conn = std::shared_ptr<UniversalConnection>;
    using Msg = std::shared_ptr<matrixserver::MatrixServerMessage>;

    dispatcher_.registerHandler(matrixserver::registerApp, [this](Conn c, Msg m) { handleRegisterApp(std::move(c), std::move(m)); });

    dispatcher_.registerHandler(matrixserver::getServerInfo, [this](Conn c, Msg m) { handleGetServerInfo(std::move(c), std::move(m)); });

    dispatcher_.registerHandler(matrixserver::appPause, [this](Conn c, Msg m) { handleAppPause(std::move(c), std::move(m)); });

    dispatcher_.registerHandler(matrixserver::appAlive, [this](Conn c, Msg m) { handleAppAlive(std::move(c), std::move(m)); });

    dispatcher_.registerHandler(matrixserver::appResume, [this](Conn c, Msg m) { handleAppResume(std::move(c), std::move(m)); });

    dispatcher_.registerHandler(matrixserver::appKill, [this](Conn c, Msg m) { handleAppKill(std::move(c), std::move(m)); });

    // Both requestScreenAccess and setScreenFrame signal the condvar.
    dispatcher_.registerHandler(matrixserver::requestScreenAccess, [this](Conn c, Msg m) { handleScreenAccess(std::move(c), std::move(m)); });

    dispatcher_.registerHandler(matrixserver::setScreenFrame, [this](Conn c, Msg m) { handleScreenAccess(std::move(c), std::move(m)); });

    dispatcher_.registerHandler(matrixserver::joystickData, [this](Conn c, Msg m) { handleJoystickData(std::move(c), std::move(m)); });

    dispatcher_.registerHandler(matrixserver::imuData, [this](Conn c, Msg m) { handleImuData(std::move(c), std::move(m)); });

    dispatcher_.registerHandler(matrixserver::audioDataMessage, [this](Conn c, Msg m) { handleAudioData(std::move(c), std::move(m)); });

    dispatcher_.registerHandler(matrixserver::setAppParam, [this](Conn c, Msg m) { handleSetAppParam(std::move(c), std::move(m)); });

    dispatcher_.registerHandler(matrixserver::getAppParams, [this](Conn c, Msg m) { handleGetAppParams(std::move(c), std::move(m)); });
}

// ---------------------------------------------------------------------------
// handleRequest — thin shim; real work is in the per-type handlers below
// ---------------------------------------------------------------------------

void MatrixApplication::handleRequest(std::shared_ptr<UniversalConnection> conn, std::shared_ptr<matrixserver::MatrixServerMessage> message) {
    BOOST_LOG_TRIVIAL(trace) << "[Application] handleRequest called";
    dispatcher_.dispatch(std::move(conn), std::move(message));
}

// ---------------------------------------------------------------------------
// Per-message-type handlers
// ---------------------------------------------------------------------------

void MatrixApplication::handleRegisterApp(std::shared_ptr<UniversalConnection> conn, std::shared_ptr<matrixserver::MatrixServerMessage> msg) {
    if (msg->status() == matrixserver::success) {
        BOOST_LOG_TRIVIAL(debug) << "[Application] Register at Server successfull";
        appId = msg->appid();

        // Send parameter schema if any are registered
        auto schemaMsg = std::make_shared<matrixserver::MatrixServerMessage>();
        schemaMsg->set_messagetype(matrixserver::appParamSchema);
        schemaMsg->set_appid(appId);
        auto schema = params.toSchema(appName);
        schemaMsg->mutable_appparamschema()->CopyFrom(schema);
        conn->sendMessage(schemaMsg);

        auto response = std::make_shared<matrixserver::MatrixServerMessage>();
        response->set_messagetype(matrixserver::getServerInfo);
        response->set_appid(appId);
        conn->sendMessage(response);
    }
}

void MatrixApplication::handleGetServerInfo(std::shared_ptr<UniversalConnection> /*conn*/, std::shared_ptr<matrixserver::MatrixServerMessage> msg) {
    BOOST_LOG_TRIVIAL(debug) << "[Application] ServerInfo received, setup complete!";
    serverConfig.Clear();
    serverConfig.CopyFrom(msg->serverconfig());
    for (const auto& screenInfo : serverConfig.screeninfo()) {
        screens.push_back(std::make_shared<Screen>(screenInfo.width(), screenInfo.height(), screenInfo.screenid()));
    }
    runner_.setState(AppState::running);
}

void MatrixApplication::handleAppPause(std::shared_ptr<UniversalConnection> conn, std::shared_ptr<matrixserver::MatrixServerMessage> /*msg*/) {
    auto response = std::make_shared<matrixserver::MatrixServerMessage>();
    response->set_messagetype(matrixserver::appPause);
    response->set_appid(appId);
    if (pause()) {
        response->set_status(matrixserver::success);
        BOOST_LOG_TRIVIAL(debug) << "app Paused";
    } else {
        response->set_status(matrixserver::error);
    }
    conn->sendMessage(response);
}

void MatrixApplication::handleAppAlive(std::shared_ptr<UniversalConnection> conn, std::shared_ptr<matrixserver::MatrixServerMessage> /*msg*/) {
    auto response = std::make_shared<matrixserver::MatrixServerMessage>();
    response->set_messagetype(matrixserver::appAlive);
    response->set_appid(appId);
    auto state = runner_.getState();
    if (state == AppState::running || state == AppState::paused)
        response->set_status(matrixserver::success);
    else
        response->set_status(matrixserver::error);
    conn->sendMessage(response);
}

void MatrixApplication::handleAppResume(std::shared_ptr<UniversalConnection> conn, std::shared_ptr<matrixserver::MatrixServerMessage> /*msg*/) {
    auto response = std::make_shared<matrixserver::MatrixServerMessage>();
    response->set_messagetype(matrixserver::appResume);
    response->set_appid(appId);
    if (resume())
        response->set_status(matrixserver::success);
    else
        response->set_status(matrixserver::error);
    conn->sendMessage(response);
}

void MatrixApplication::handleAppKill(std::shared_ptr<UniversalConnection> conn, std::shared_ptr<matrixserver::MatrixServerMessage> /*msg*/) {
    auto response = std::make_shared<matrixserver::MatrixServerMessage>();
    response->set_messagetype(matrixserver::appKill);
    response->set_appid(appId);
    response->set_status(matrixserver::success);
    conn->sendMessage(response);
    BOOST_LOG_TRIVIAL(debug) << "app killed";
    stop();
}

void MatrixApplication::handleScreenAccess(std::shared_ptr<UniversalConnection> /*conn*/,
                                           std::shared_ptr<matrixserver::MatrixServerMessage> /*msg*/) {
    // Both requestScreenAccess and setScreenFrame use this handler.
    // Signal the LoopRunner condvar that the server has acknowledged our frame.
    runner_.signalScreenAccess();
}

void MatrixApplication::handleJoystickData(std::shared_ptr<UniversalConnection> /*conn*/, std::shared_ptr<matrixserver::MatrixServerMessage> msg) {
    for (int i = 0; i < msg->joystickdata_size(); ++i) {
        const auto& joystickData = msg->joystickdata(i);
        BOOST_LOG_TRIVIAL(debug) << "[Application] Received joystickData for ID: " << joystickData.joystickid()
                                 << " data: " << joystickData.ShortDebugString();
        Joystick::updateSimulatorState(joystickData);
    }
}

void MatrixApplication::handleImuData(std::shared_ptr<UniversalConnection> /*conn*/, std::shared_ptr<matrixserver::MatrixServerMessage> msg) {
    ImuSample sample;
    sample.ax = msg->imudata().accelx();
    sample.ay = msg->imudata().accely();
    sample.az = msg->imudata().accelz();
    sample.gx = msg->imudata().gyrox();
    sample.gy = msg->imudata().gyroy();
    sample.gz = msg->imudata().gyroz();
    inputState_.setImu(sample);
}

void MatrixApplication::handleAudioData(std::shared_ptr<UniversalConnection> /*conn*/, std::shared_ptr<matrixserver::MatrixServerMessage> msg) {
    if (msg->has_audiodata()) {
        uint8_t volume = static_cast<uint8_t>(msg->audiodata().volume());
        std::vector<uint8_t> frequencies;
        frequencies.reserve(static_cast<std::size_t>(msg->audiodata().frequencybands_size()));
        for (int i = 0; i < msg->audiodata().frequencybands_size(); ++i) {
            frequencies.push_back(static_cast<uint8_t>(msg->audiodata().frequencybands(i)));
        }
        inputState_.setAudio(volume, frequencies);
    }
}

void MatrixApplication::handleSetAppParam(std::shared_ptr<UniversalConnection> /*conn*/, std::shared_ptr<matrixserver::MatrixServerMessage> msg) {
    if (msg->has_appparamupdate()) {
        params.applyUpdate(msg->appparamupdate());
    }
}

void MatrixApplication::handleGetAppParams(std::shared_ptr<UniversalConnection> conn, std::shared_ptr<matrixserver::MatrixServerMessage> /*msg*/) {
    auto response = std::make_shared<matrixserver::MatrixServerMessage>();
    response->set_messagetype(matrixserver::appParamValues);
    response->set_status(matrixserver::success);
    response->set_appid(appId);
    auto values = params.toValues(appName);
    response->mutable_appparamvalues()->CopyFrom(values);
    conn->sendMessage(response);
}

// ---------------------------------------------------------------------------
// Accessors / lifecycle
// ---------------------------------------------------------------------------

int MatrixApplication::getFps() { return runner_.getFps(); }

void MatrixApplication::setFps(int setFps) {
    if (setFps <= MAXFPS && setFps >= MINFPS) {
        runner_.setFps(setFps);
    } else if (setFps == 0) {
        runner_.setFps(DEFAULTFPS);
    }
}

AppState MatrixApplication::getAppState() { return runner_.getState(); }

float MatrixApplication::getLoad() { return runner_.getLoad(); }

void MatrixApplication::start() {
    // Register at the server here (not in the constructor) so that derived-class
    // constructors have already run and any params.register*() calls are done
    // before we send the schema to the server.
    registerAtServer();
    runner_.start([this]() { return loop(); },
                  [this]() {
                      renderToScreens();
                      checkConnection();
                  });
}

bool MatrixApplication::pause() {
    if (runner_.getState() == AppState::running) {
        runner_.setState(AppState::paused);
        return true;
    }
    return false;
}

bool MatrixApplication::resume() {
    if (runner_.getState() == AppState::paused) {
        runner_.setState(AppState::running);
        return true;
    }
    return false;
}

void MatrixApplication::stop() {
    runner_.stop();
    // Stop io_context so ioThread can exit cleanly.
    // Guard against self-join: stop() may be called from within the IO thread
    // (e.g. via handleRequest appKill). In that case we detach rather than join
    // so that std::thread::~thread() does not call std::terminate().
    io_context.stop();
    if (ioThread && ioThread->joinable()) {
        if (ioThread->get_id() != std::this_thread::get_id()) {
            ioThread->join();
        } else {
            // Called from the IO thread itself — detach so the thread outlives this
            // stop() call and cleans up naturally when io_context.run() returns.
            ioThread->detach();
        }
    }
    ioThread.reset();
    BOOST_LOG_TRIVIAL(debug) << "[Application] App stopped";
}

MatrixApplication::~MatrixApplication() { stop(); }

int MatrixApplication::getBrightness() { return serverConfig.globalscreenbrightness(); }

void MatrixApplication::setBrightness(int setBrightness) {
    serverConfig.set_globalscreenbrightness(setBrightness);
    updateBrightness = true;
}

long MatrixApplication::micros() {
    struct timeval tp;
    gettimeofday(&tp, nullptr);
    long us = tp.tv_sec * 1000000 + tp.tv_usec;
    return us;
}
