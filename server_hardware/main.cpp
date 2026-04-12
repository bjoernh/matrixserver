#include <boost/log/trivial.hpp>
#include <memory>
#include <unistd.h>

#include <Server.h>
#include <ServerSetup.h>
#include <WebSocketSimulatorRenderer.h>

#if defined(BACKEND_FPGA_FTDI)
#include <FPGARendererFTDI.h>
using HWRenderer = FPGARendererFTDI;
constexpr auto kHwType = ServerSetup::HardwareType::FPGA_FTDI;
#elif defined(BACKEND_FPGA_RPISPI)
#include <FPGARendererRPISPI.h>
using HWRenderer = FPGARendererRPISPI;
constexpr auto kHwType = ServerSetup::HardwareType::FPGA_RPISPI;
#elif defined(BACKEND_RGB_MATRIX)
#include <RGBMatrixRenderer.h>
using HWRenderer = RGBMatrixRenderer;
constexpr auto kHwType = ServerSetup::HardwareType::RGB_MATRIX;
#else
#error "No HARDWARE_BACKEND defined. Use -DHARDWARE_BACKEND=FPGA_FTDI|FPGA_RPISPI|RGB_MATRIX"
#endif

int main(int argc, char **argv) {
  try {
    matrixserver::ServerConfig serverConfig;
    ServerSetup::handleServerConfig(argc, argv, serverConfig, kHwType);

    BOOST_LOG_TRIVIAL(info) << "ServerConfig: " << std::endl
                            << serverConfig.DebugString() << std::endl;

    auto screens = ServerSetup::createScreensFromConfig(serverConfig);
    auto renderer = std::make_shared<HWRenderer>(screens);

    BOOST_LOG_TRIVIAL(debug) << "[Server] Renderer initialized";

    Server server(renderer, serverConfig);

    // Add WebSocket renderer for webapp parameter control (no pixel streaming)
    std::string simPort = serverConfig.simulatorport();
    if (simPort.empty()) simPort = "1337";
    auto wsRenderer = std::make_shared<WebSocketSimulatorRenderer>(
        screens, simPort, false);
    server.addRenderer(wsRenderer);

    int tickMs = serverConfig.tickintervalms();
    if (tickMs <= 0) tickMs = 1000;

    while (server.tick()) {
      usleep(tickMs * 1000);
    }
  } catch (const std::exception &e) {
    BOOST_LOG_TRIVIAL(fatal) << "[Server] Fatal error: " << e.what();
    return 1;
  }
  return 0;
}
