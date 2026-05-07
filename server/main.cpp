#include "RendererFactory.h"

#include <ServerBootstrap.h>
#include <boost/program_options.hpp>
#include <iostream>
#include <stdexcept>
#include <string>

namespace po = boost::program_options;

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

    ServerBootstrap::HardwareRendererFactory factory;
    ServerSetup::HardwareType hwType;

    try {
        factory = makeHardwareRendererFactory(backend);
        hwType  = hardwareTypeForBackend(backend);
    } catch (const std::invalid_argument &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return ServerBootstrap::runServer(argc, argv, hwType, factory);
}
