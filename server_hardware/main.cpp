#include <ServerBootstrap.h>

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
    auto factory = [](const std::vector<std::shared_ptr<Screen>> &screens)
        -> std::shared_ptr<IRenderer> {
        return std::make_shared<HWRenderer>(screens);
    };
    return ServerBootstrap::runServer(argc, argv, kHwType, factory);
}
