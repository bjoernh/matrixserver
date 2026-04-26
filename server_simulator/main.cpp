#include <ServerBootstrap.h>

int main(int argc, char **argv) {
    return ServerBootstrap::runServer(
        argc, argv, ServerSetup::HardwareType::Simulator, nullptr);
}
