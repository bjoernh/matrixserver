#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/trivial.hpp>
#include <chrono>
#include <iostream>
#include <random>
#include <sys/wait.h>
#include <vector>

#include "Server.h"

// ---------------------------------------------------------------------------
// Destructor
// ---------------------------------------------------------------------------

Server::~Server() {
    // Stop the io_context so ioThread exits cleanly.
    ioWork.reset(); // drop work guard so run() can return
    ioContext.stop();
    if (ioThread && ioThread->joinable()) {
        ioThread->join();
    }
}

// ---------------------------------------------------------------------------
// Lookup
// ---------------------------------------------------------------------------

std::shared_ptr<App> Server::getAppByID(int searchID) {
    std::lock_guard<std::mutex> lock(appsMutex);
    for (auto &app : apps) {
        if (app->getAppId() == searchID) {
            return app;
        }
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

Server::Server(std::shared_ptr<IRenderer> setRenderer, matrixserver::ServerConfig &setServerConfig)
    : ioContext(),
      ioWork(new boost::asio::executor_work_guard<boost::asio::io_context::executor_type>(boost::asio::make_work_guard(ioContext))),
      serverConfig(setServerConfig),
      tcpServer(ioContext, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), std::stoi(setServerConfig.serverconnection().serverport()))),
      ipcServer("matrixserver"),
      joystickmngr(8) {
    boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::debug);

    registry_.setMessageCallback([this](std::shared_ptr<matrixserver::MatrixServerMessage> msg) {
        if (msg->messagetype() == matrixserver::getServerInfo) {
            auto response = std::make_shared<matrixserver::MatrixServerMessage>();
            response->set_messagetype(matrixserver::getServerInfo);
            auto *tempConfig = new matrixserver::ServerConfig();
            tempConfig->CopyFrom(serverConfig);
            response->set_allocated_serverconfig(tempConfig);
            registry_.broadcastMessage(response);

            std::lock_guard<std::mutex> lock(appsMutex);
            if (!apps.empty()) {
                auto schemaMsg = std::make_shared<matrixserver::MatrixServerMessage>();
                schemaMsg->set_appid(apps.back()->getAppId());
                schemaMsg->set_messagetype(matrixserver::appParamSchema);
                auto schema = apps.back()->getParamSchema();
                schemaMsg->mutable_appparamschema()->CopyFrom(schema);
                registry_.broadcastMessage(schemaMsg);
            }
        } else if (msg->messagetype() == matrixserver::imuData || msg->messagetype() == matrixserver::audioDataMessage ||
                   msg->messagetype() == matrixserver::joystickData || msg->messagetype() == matrixserver::setAppParam ||
                   msg->messagetype() == matrixserver::getAppParams) {
            std::lock_guard<std::mutex> lock(appsMutex);
            if (msg->messagetype() == matrixserver::setAppParam || msg->messagetype() == matrixserver::getAppParams) {
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

    registry_.add(setRenderer);
    tcpServer.setAcceptCallback(std::bind(&Server::newConnectionCallback, this, std::placeholders::_1));
    ipcServer.setAcceptCallback(std::bind(&Server::newConnectionCallback, this, std::placeholders::_1));

    setupDispatcher();

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

// ---------------------------------------------------------------------------
// Dispatcher setup
// ---------------------------------------------------------------------------

void Server::setupDispatcher() {
    using namespace std::placeholders;
    dispatcher_.registerHandler(matrixserver::registerApp, std::bind(&Server::handleRegisterApp, this, _1, _2));
    dispatcher_.registerHandler(matrixserver::getServerInfo, std::bind(&Server::handleGetServerInfo, this, _1, _2));
    dispatcher_.registerHandler(matrixserver::requestScreenAccess, std::bind(&Server::handleRequestScreenAccess, this, _1, _2));
    dispatcher_.registerHandler(matrixserver::setScreenFrame, std::bind(&Server::handleSetScreenFrame, this, _1, _2));
    // appAlive / appPause / appResume / appKill all share the same lifecycle handler
    dispatcher_.registerHandler(matrixserver::appAlive, std::bind(&Server::handleAppLifecycle, this, _1, _2));
    dispatcher_.registerHandler(matrixserver::appPause, std::bind(&Server::handleAppLifecycle, this, _1, _2));
    dispatcher_.registerHandler(matrixserver::appResume, std::bind(&Server::handleAppLifecycle, this, _1, _2));
    dispatcher_.registerHandler(matrixserver::appKill, std::bind(&Server::handleAppLifecycle, this, _1, _2));
    dispatcher_.registerHandler(matrixserver::appParamSchema, std::bind(&Server::handleAppParamSchema, this, _1, _2));
    dispatcher_.registerHandler(matrixserver::appParamValues, std::bind(&Server::handleAppParamValues, this, _1, _2));
}

// ---------------------------------------------------------------------------
// Connection callbacks
// ---------------------------------------------------------------------------

void Server::newConnectionCallback(std::shared_ptr<UniversalConnection> connection) {
    BOOST_LOG_TRIVIAL(debug) << "[matrixserver] NEW SocketConnection CALLBACK!";
    connection->setReceiveCallback(std::bind(&Server::handleRequest, this, std::placeholders::_1, std::placeholders::_2));
    connections.push_back(connection);
}

// ---------------------------------------------------------------------------
// handleRequest — thin dispatcher wrapper
// ---------------------------------------------------------------------------

void Server::handleRequest(std::shared_ptr<UniversalConnection> connection, std::shared_ptr<matrixserver::MatrixServerMessage> message) {
    dispatcher_.dispatch(connection, message);
}

// ---------------------------------------------------------------------------
// Per-message-type handlers
// ---------------------------------------------------------------------------

void Server::handleRegisterApp(std::shared_ptr<UniversalConnection> conn, std::shared_ptr<matrixserver::MatrixServerMessage> msg) {
    if (msg->appid() == 0) {
        BOOST_LOG_TRIVIAL(debug) << "[matrixserver] register new App request received";
        int newAppId;
        {
            std::lock_guard<std::mutex> lock(appsMutex);
            apps.push_back(std::make_shared<App>(conn));
            newAppId = apps.back()->getAppId();
        }
        auto response = std::make_shared<matrixserver::MatrixServerMessage>();
        response->set_appid(newAppId);
        response->set_messagetype(matrixserver::registerApp);
        response->set_status(matrixserver::success);
        conn->sendMessage(response);
    }
}

void Server::handleGetServerInfo(std::shared_ptr<UniversalConnection> conn, std::shared_ptr<matrixserver::MatrixServerMessage> /*msg*/) {
    BOOST_LOG_TRIVIAL(debug) << "[matrixserver] get ServerInfo request received";
    auto response = std::make_shared<matrixserver::MatrixServerMessage>();
    response->set_messagetype(matrixserver::getServerInfo);
    auto *tempServerConfig = new matrixserver::ServerConfig();
    tempServerConfig->CopyFrom(serverConfig);
    response->set_allocated_serverconfig(tempServerConfig);
    conn->sendMessage(response);
}

void Server::handleRequestScreenAccess(std::shared_ptr<UniversalConnection> /*conn*/, std::shared_ptr<matrixserver::MatrixServerMessage> /*msg*/) {
    // TODO App level logic: set App on top, pause all other Apps which aren't
    // on top any more.
}

void Server::handleSetScreenFrame(std::shared_ptr<UniversalConnection> conn, std::shared_ptr<matrixserver::MatrixServerMessage> msg) {
    bool isTopApp = false;
    {
        std::lock_guard<std::mutex> lock(appsMutex);
        if (!apps.empty() && msg->appid() == apps.back()->getAppId()) {
            isTopApp = true;
        }
    }
    if (isTopApp) {
        registry_.forEachRenderer([this, msg, conn](std::shared_ptr<IRenderer> renderer) {
            for (const auto &screenInfo : msg->screendata()) {
                int sid = screenInfo.screenid();
                if (sid < 0 || sid >= static_cast<int>(serverConfig.screeninfo_size())) {
                    BOOST_LOG_TRIVIAL(warning) << "[Server] Invalid screen ID: " << sid;
                    continue;
                }
                renderer->setScreenData(sid, (Color *)screenInfo.framedata().data());
            }
            auto usStart = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
            renderer->render();
            auto response = std::make_shared<matrixserver::MatrixServerMessage>();
            response->set_messagetype(matrixserver::setScreenFrame);
            response->set_status(matrixserver::success);
            conn->sendMessage(response);
            auto usTotal = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()) - usStart;
            (void)usTotal; // reserved for debug logging
            if (msg->has_serverconfig()) {
                renderer->setGlobalBrightness(msg->serverconfig().globalscreenbrightness());
            }
        });
    } else {
        // Send app to pause (not top app).
        BOOST_LOG_TRIVIAL(debug) << "[Server] send app " << msg->appid() << " to pause";
        auto killMsg = std::make_shared<matrixserver::MatrixServerMessage>();
        killMsg->set_messagetype(matrixserver::appKill);
        conn->sendMessage(killMsg);
    }
}

void Server::handleAppLifecycle(std::shared_ptr<UniversalConnection> /*conn*/, std::shared_ptr<matrixserver::MatrixServerMessage> msg) {
    BOOST_LOG_TRIVIAL(debug) << "[Server] appkill " << msg->appid() << " successfull";
    std::lock_guard<std::mutex> lock(appsMutex);
    apps.erase(std::remove_if(apps.begin(), apps.end(),
                              [&msg](const std::shared_ptr<App> &a) {
                                  if (a->getAppId() == msg->appid()) {
                                      BOOST_LOG_TRIVIAL(debug) << "[Server] App " << msg->appid() << " deleted";
                                      return true;
                                  }
                                  return false;
                              }),
               apps.end());
}

void Server::handleAppParamSchema(std::shared_ptr<UniversalConnection> /*conn*/, std::shared_ptr<matrixserver::MatrixServerMessage> msg) {
    BOOST_LOG_TRIVIAL(debug) << "[Server] App parameter schema received";
    {
        std::lock_guard<std::mutex> lock(appsMutex);
        for (auto &app : apps) {
            if (app->getAppId() == msg->appid()) {
                app->setParamSchema(msg->appparamschema());
                break;
            }
        }
    }
    // Forward to all bidirectional renderers (messaging-capable only).
    registry_.broadcastMessage(msg);
}

void Server::handleAppParamValues(std::shared_ptr<UniversalConnection> /*conn*/, std::shared_ptr<matrixserver::MatrixServerMessage> msg) {
    // Forward current values to all bidirectional renderers.
    registry_.broadcastMessage(msg);
}

// ---------------------------------------------------------------------------
// tick
// ---------------------------------------------------------------------------

bool Server::tick() {
    if (joystickmngr.getButtonPress(7)) {
        std::lock_guard<std::mutex> lock(appsMutex);
        if (!apps.empty()) {
            BOOST_LOG_TRIVIAL(debug) << "kill current app" << std::endl;
            auto msg = std::make_shared<matrixserver::MatrixServerMessage>();
            msg->set_messagetype(matrixserver::appKill);
            apps.back()->sendMsg(msg);
        }
    }
    joystickmngr.clearAllButtonPresses();

    {
        std::lock_guard<std::mutex> lock(appsMutex);
        if (apps.empty()) {
            defaultAppLauncher_.launchIfNotRunning();
        }
        if (!apps.empty()) {
            defaultAppLauncher_.markAppsPresent();
        }

        apps.erase(std::remove_if(apps.begin(), apps.end(),
                                  [](const std::shared_ptr<App> &a) {
                                      bool dead = a->getConnection()->isDead();
                                      if (dead) {
                                          BOOST_LOG_TRIVIAL(debug) << "[matrixserver] App " << a->getAppId() << " deleted";
                                      }
                                      return dead;
                                  }),
                   apps.end());
    }

    connections.erase(std::remove_if(connections.begin(), connections.end(),
                                     [](std::shared_ptr<UniversalConnection> con) {
                                         bool dead = con->isDead();
                                         if (dead) {
                                             BOOST_LOG_TRIVIAL(debug) << "[matrixserver] Connection deleted";
                                         }
                                         return dead;
                                     }),
                      connections.end());
    return true;
}

// ---------------------------------------------------------------------------
// stopDefaultApp — delegates to DefaultAppLauncher
// ---------------------------------------------------------------------------

void Server::stopDefaultApp() { defaultAppLauncher_.stop(); }

// ---------------------------------------------------------------------------
// addRenderer
// ---------------------------------------------------------------------------

void Server::addRenderer(std::shared_ptr<IRenderer> newRenderer) { registry_.add(newRenderer); }
