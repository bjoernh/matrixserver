#include "RendererFactory.h"

#include <IRenderer.h>
#include <Screen.h>
#include <format>
#include <memory>
#include <stdexcept>

#ifdef HAVE_FPGA_FTDI
#include <FPGARendererFTDI.h>
#endif

#ifdef HAVE_FPGA_RPISPI
#include <FPGARendererRPISPI.h>
#endif

#ifdef HAVE_RGB_MATRIX
#include <RGBMatrixRenderer.h>
#endif

std::vector<std::string> availableBackends() {
    std::vector<std::string> backends = {"simulator"};
#ifdef HAVE_FPGA_FTDI
    backends.push_back("fpga-ftdi");
#endif
#ifdef HAVE_FPGA_RPISPI
    backends.push_back("fpga-rpispi");
#endif
#ifdef HAVE_RGB_MATRIX
    backends.push_back("rgb-matrix");
#endif
    return backends;
}

static std::string makeAvailableList() {
    auto backends = availableBackends();
    std::string result;
    for (size_t i = 0; i < backends.size(); ++i) {
        if (i > 0) result += ", ";
        result += backends[i];
    }
    return result;
}

ServerBootstrap::HardwareRendererFactory makeHardwareRendererFactory(std::string_view backend) {
    if (backend == "simulator") {
        return nullptr;
    }

#ifdef HAVE_FPGA_FTDI
    if (backend == "fpga-ftdi") {
        return [](const std::vector<std::shared_ptr<Screen>> &screens) -> std::shared_ptr<IRenderer> {
            return std::make_shared<FPGARendererFTDI>(screens);
        };
    }
#endif

#ifdef HAVE_FPGA_RPISPI
    if (backend == "fpga-rpispi") {
        return [](const std::vector<std::shared_ptr<Screen>> &screens) -> std::shared_ptr<IRenderer> {
            return std::make_shared<FPGARendererRPISPI>(screens);
        };
    }
#endif

#ifdef HAVE_RGB_MATRIX
    if (backend == "rgb-matrix") {
        return [](const std::vector<std::shared_ptr<Screen>> &screens) -> std::shared_ptr<IRenderer> {
            return std::make_shared<RGBMatrixRenderer>(screens);
        };
    }
#endif

    throw std::invalid_argument(
        std::format("Unknown or not-compiled-in backend '{}'. Available backends in this binary: {}",
                    backend, makeAvailableList()));
}

ServerSetup::HardwareType hardwareTypeForBackend(std::string_view backend) {
    if (backend == "simulator")   return ServerSetup::HardwareType::Simulator;
    if (backend == "fpga-ftdi")   return ServerSetup::HardwareType::FPGA_FTDI;
    if (backend == "fpga-rpispi") return ServerSetup::HardwareType::FPGA_RPISPI;
    if (backend == "rgb-matrix")  return ServerSetup::HardwareType::RGB_MATRIX;

    throw std::invalid_argument(
        std::format("Unknown backend '{}'. Available backends in this binary: {}",
                    backend, makeAvailableList()));
}
