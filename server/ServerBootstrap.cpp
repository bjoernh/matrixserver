#include "ServerBootstrap.h"

#include <Server.h>
#include <WebSocketSimulatorRenderer.h>
#include <boost/log/trivial.hpp>

#include <atomic>
#include <csignal>
#include <unistd.h>

namespace {

std::atomic<bool> g_shutdown{false};
void signalHandler(int) { g_shutdown = true; }

} // namespace

namespace ServerBootstrap {

int runServer(int argc, char **argv,
              ServerSetup::HardwareType hwType,
              HardwareRendererFactory makeHardwareRenderer) {
    g_shutdown = false;
    std::signal(SIGTERM, signalHandler);
    std::signal(SIGINT,  signalHandler);

    try {
        matrixserver::ServerConfig serverConfig;
        ServerSetup::handleServerConfig(argc, argv, serverConfig, hwType);

        BOOST_LOG_TRIVIAL(info) << "ServerConfig: " << std::endl
                                << serverConfig.DebugString() << std::endl;

        auto screens = ServerSetup::createScreensFromConfig(serverConfig);

        std::string simPort = serverConfig.simulatorport();
        if (simPort.empty())
            simPort = "1337";

        std::shared_ptr<IRenderer> primaryRenderer;

        if (makeHardwareRenderer) {
            primaryRenderer = makeHardwareRenderer(screens);
        }

        if (primaryRenderer) {
            // Hardware mode: primary renderer drives pixels; WebSocket is a
            // parameter-only secondary channel (streamPixels=false).
            BOOST_LOG_TRIVIAL(debug) << "[Server] Renderer initialized";

            Server server(primaryRenderer, serverConfig);

            auto wsRenderer = std::make_shared<WebSocketSimulatorRenderer>(
                screens, simPort, false);
            server.addRenderer(wsRenderer);

            int tickMs = serverConfig.tickintervalms();
            if (tickMs <= 0) tickMs = 1000;

            while (!g_shutdown && server.tick())
                usleep(tickMs * 1000);

            server.stopDefaultApp();
        } else {
            // Simulator mode: single WebSocket renderer with pixel streaming on.
            auto wsRenderer =
                std::make_shared<WebSocketSimulatorRenderer>(screens, simPort);

            Server server(wsRenderer, serverConfig);

            int tickMs = serverConfig.tickintervalms();
            if (tickMs <= 0) tickMs = 1000;

            while (!g_shutdown && server.tick())
                usleep(tickMs * 1000);

            server.stopDefaultApp();
        }
    } catch (const std::exception &e) {
        BOOST_LOG_TRIVIAL(fatal) << "[Server] Fatal error: " << e.what();
        return 1;
    }
    return 0;
}

} // namespace ServerBootstrap
