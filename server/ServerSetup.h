#ifndef MATRIXSERVER_SERVERSETUP_H
#define MATRIXSERVER_SERVERSETUP_H

#include <matrixserver.pb.h>
#include <string>

namespace ServerSetup {

/**
 * Initializes the server configuration.
 * Parses command-line arguments using boost::program_options to allow
 * --config, --address, and --use-deprecated-tcp-connection.
 * If no config file is provided or it doesn't exist, it creates a default one.
 *
 * @param argc Command line argument count
 * @param argv Command line arguments
 * @param serverConfig The configuration object to populate
 * @param useDeprecatedTcp Output boolean flag for simulator legacy TCP mode
 */
void handleServerConfig(int argc, char **argv,
                        matrixserver::ServerConfig &serverConfig,
                        bool &useDeprecatedTcp);

/**
 * Creates the default cube configuration.
 */
void createDefaultCubeConfig(matrixserver::ServerConfig &serverConfig);

} // namespace ServerSetup

#endif // MATRIXSERVER_SERVERSETUP_H
