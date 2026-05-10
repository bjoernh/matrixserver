#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <csignal>
#include <unistd.h>
#include <boost/program_options.hpp>
#include <boost/log/trivial.hpp>

#include "RendererFactory.h"
#include <Server.h>
#include <ServerSetup.h>
#include <WebSocketSimulatorRenderer.h>

namespace po = boost::program_options;

static std::atomic<bool> g_shutdown{false};
static void signalHandler(int) { g_shutdown = true; }

int main(int argc, char **argv) {
    std::string backend = "simulator";

    try {
        po::options_description desc("Backend selection");
        desc.add_options()("backend", po::value<std::string>(&backend)->default_value("simulator"),
                           "Renderer backend to use");

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).options(desc).allow_unregistered().run(), vm);
        po::notify(vm);
    } catch (const std::exception &e) {
        std::cerr << "Error parsing --backend: " << e.what() << "\n";
        return 1;
    }

    HardwareRendererFactory factory;
    ServerSetup::HardwareType hwType;

    try {
        factory = makeHardwareRendererFactory(backend);
        hwType  = hardwareTypeForBackend(backend);
    } catch (const std::invalid_argument &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    std::signal(SIGTERM, signalHandler);
    std::signal(SIGINT,  signalHandler);

    try {
        matrixserver::ServerConfig serverConfig;
        ServerSetup::handleServerConfig(argc, argv, serverConfig, hwType);

        BOOST_LOG_TRIVIAL(info) << "ServerConfig: " << std::endl
                                << serverConfig.DebugString() << std::endl;

        auto screens = ServerSetup::createScreensFromConfig(serverConfig);

        std::string simPort = serverConfig.simulatorport();
        if (simPort.empty()) simPort = "1337";

        std::shared_ptr<IRenderer> primaryRenderer;
        if (factory) {
            primaryRenderer = factory(screens);
        }

        if (primaryRenderer) {
            // Hardware mode
            BOOST_LOG_TRIVIAL(debug) << "[Server] Renderer initialized";
            Server server(primaryRenderer, serverConfig);

            // Add WebSocket renderer for webapp parameter control (no pixel streaming)
            auto wsRenderer = std::make_shared<WebSocketSimulatorRenderer>(
                screens, simPort, false);
            server.addRenderer(wsRenderer);

            int tickMs = serverConfig.tickintervalms();
            if (tickMs <= 0) tickMs = 1000;

            while (!g_shutdown && server.tick()) {
                usleep(tickMs * 1000);
            }
            server.stopDefaultApp();
        } else {
            // Simulator mode
            auto wsRenderer = std::make_shared<WebSocketSimulatorRenderer>(screens, simPort);
            Server server(wsRenderer, serverConfig);

            int tickMs = serverConfig.tickintervalms();
            if (tickMs <= 0) tickMs = 1000;

            while (!g_shutdown && server.tick()) {
                usleep(tickMs * 1000);
            }
            server.stopDefaultApp();
        }
    } catch (const std::exception &e) {
        BOOST_LOG_TRIVIAL(fatal) << "[Server] Fatal error: " << e.what();
        return 1;
    }

    return 0;
}
