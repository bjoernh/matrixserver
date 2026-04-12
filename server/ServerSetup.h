#ifndef MATRIXSERVER_SERVERSETUP_H
#define MATRIXSERVER_SERVERSETUP_H

#include <Screen.h>
#include <matrixserver.pb.h>
#include <memory>
#include <string>
#include <vector>

namespace ServerSetup {

enum class HardwareType { Simulator, FPGA_FTDI, FPGA_RPISPI, RGB_MATRIX };

/**
 * Initializes the server configuration.
 * Parses command-line arguments using boost::program_options to allow
 * --config and --address.
 * If no config file is provided or it doesn't exist, it creates a default one.
 *
 * @param argc Command line argument count
 * @param argv Command line arguments
 * @param serverConfig The configuration object to populate
 */
void handleServerConfig(int argc, char **argv,
                        matrixserver::ServerConfig &serverConfig,
                        HardwareType hwType = HardwareType::Simulator);

/**
 * Creates the default cube configuration with hardware-specific screen
 * orientation defaults.
 */
void createDefaultCubeConfig(matrixserver::ServerConfig &serverConfig,
                             HardwareType hwType = HardwareType::Simulator);

/**
 * Creates Screen objects from the server configuration, reading offsetX,
 * offsetY, and screenRotation from each ScreenInfo entry.
 */
std::vector<std::shared_ptr<Screen>>
createScreensFromConfig(const matrixserver::ServerConfig &serverConfig);

} // namespace ServerSetup

#endif // MATRIXSERVER_SERVERSETUP_H
