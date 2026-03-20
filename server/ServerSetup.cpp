#include "ServerSetup.h"

#include <boost/log/trivial.hpp>
#include <boost/program_options.hpp>
#include <exception>
#include <fstream>
#include <iostream>
#include <string>

#include <google/protobuf/util/json_util.h>

namespace po = boost::program_options;

namespace ServerSetup {

void createDefaultCubeConfig(matrixserver::ServerConfig &serverConfig) {
  serverConfig.Clear();
  serverConfig.set_globalscreenbrightness(100);
  serverConfig.set_servername("matrixserver");
  matrixserver::Connection *serverConnection = new matrixserver::Connection();
  serverConnection->set_serveraddress("127.0.0.1");
  serverConnection->set_serverport("2017");
  serverConnection->set_connectiontype(
      matrixserver::Connection_ConnectionType_tcp);
  serverConfig.set_allocated_serverconnection(serverConnection);
  serverConfig.set_assemblytype(matrixserver::ServerConfig_AssemblyType_cube);
  for (int i = 0; i < 6; i++) {
    auto screenInfo = serverConfig.add_screeninfo();
    screenInfo->set_screenid(i);
    screenInfo->set_available(true);
    screenInfo->set_height(64);
    screenInfo->set_width(64);
    screenInfo->set_screenorientation(
        (matrixserver::ScreenInfo_ScreenOrientation)(i + 1));
  }
  // Included by default for the simulator, ignored by others
  serverConfig.set_simulatoraddress("127.0.0.1");
  serverConfig.set_simulatorport("1337");
}

void handleServerConfig(int argc, char **argv,
                        matrixserver::ServerConfig &serverConfig,
                        bool &useDeprecatedTcp) {
  useDeprecatedTcp = false;
  std::string configPath = "";
  std::string serverAddressOverride = "";

  try {
    po::options_description desc("Allowed options");
    desc.add_options()("help", "produce help message")(
        "config", po::value<std::string>(&configPath),
        "path to configuration file")(
        "address", po::value<std::string>(&serverAddressOverride),
        "override server address")("use-deprecated-tcp-connection",
                                   po::bool_switch(&useDeprecatedTcp),
                                   "Use legacy TCP for simulator");

    po::positional_options_description p;
    // Allow positional arguments for legacy support (e.g. simulator used to
    // take [override_address] [config_path]) But since the order was weird
    // (simulator: addr config; others: config), we'll prioritize explicit flags
    // Let's still allow a single positional argument to be the config for
    // backwards compatibility with non-simulator targets
    p.add("config", 1);

    po::variables_map vm;
    // We use parsed_options allowing unregistered options so we don't break
    // immediately if an unknown flag is passed
    po::store(po::command_line_parser(argc, argv)
                  .options(desc)
                  .positional(p)
                  .allow_unregistered()
                  .run(),
              vm);
    po::notify(vm);

    if (vm.count("help")) {
      std::string executableName = argc > 0 ? argv[0] : "matrixserver";

      // Extract just the filename if it's a path
      size_t lastSlash = executableName.find_last_of("/\\");
      if (lastSlash != std::string::npos) {
        executableName = executableName.substr(lastSlash + 1);
      }

      std::cout
          << executableName
          << " 1.0.0 - Matrix Server for rendering to LED cubes and other "
             "matrix displays.\n\n"
          << "USAGE:\n"
          << "  " << executableName << " [OPTIONS] [CONFIG_FILE_PATH]\n\n"
          << "OPTIONS:\n"
          << "  -h, --help                       Show this help message.\n"
          << "  --config <path>                  Path to configuration file. "
             "Can also be passed as a positional argument.\n"
          << "  --address <ip>                   Override the server address "
             "specified in the config.\n"
          << "  --use-deprecated-tcp-connection  Use legacy TCP connection for "
             "simulator renderer.\n\n"
          << "EXAMPLES:\n"
          << "  " << executableName << " --config myConfig.json\n"
          << "  " << executableName << " default_config.json\n"
          << "  " << executableName << " --address 192.168.1.100\n";
      exit(0);
    }
  } catch (std::exception &e) {
    BOOST_LOG_TRIVIAL(error)
        << "[ServerSetup] Error parsing command line arguments: " << e.what();
  }

  if (configPath.empty()) {
    std::ifstream defaultFile("matrixServerConfig.json");
    if (defaultFile.good()) {
      configPath = "matrixServerConfig.json";
    }
  }

  if (!configPath.empty()) {
    std::string fullPath = configPath;
    char resolved_path[PATH_MAX];
    if (realpath(configPath.c_str(), resolved_path) != NULL) {
      fullPath = std::string(resolved_path);
    }

    BOOST_LOG_TRIVIAL(info)
        << "[ServerSetup] Trying to read config from: " << fullPath;
    std::ifstream configFileReadStream(configPath);
    std::stringstream buffer;
    buffer << configFileReadStream.rdbuf();
    if (google::protobuf::util::JsonStringToMessage(buffer.str(), &serverConfig)
            .ok()) {
      BOOST_LOG_TRIVIAL(info)
          << "[ServerSetup] ServerConfig successfully loaded from: "
          << fullPath;
    } else {
      BOOST_LOG_TRIVIAL(warning)
          << "[ServerSetup] ServerConfig read failed from: " << fullPath;
    }
  } else {
    std::string configFileName = "matrixServerConfig.json";
    std::string fullPath = configFileName;
    char resolved_path[PATH_MAX];
    if (realpath(".", resolved_path) != NULL) {
      fullPath = std::string(resolved_path) + "/" + configFileName;
    }

    std::cout << "\nNo configuration file specified or found.\n"
              << "Would you like to create a default configuration file at "
              << fullPath << "? [y/N]: ";
    std::string response;
    std::getline(std::cin, response);

    if (response == "y" || response == "Y") {
      BOOST_LOG_TRIVIAL(info) << "[ServerSetup] Creating default config...";
      createDefaultCubeConfig(serverConfig);
      std::string configString;
      google::protobuf::util::JsonPrintOptions jsonOptions;
      jsonOptions.add_whitespace = true;
      if (google::protobuf::util::MessageToJsonString(
              serverConfig, &configString, jsonOptions)
              .ok()) {
        std::ofstream configFileWriteStream(configFileName,
                                            std::ios_base::trunc);
        configFileWriteStream << configString;
        configFileWriteStream.close();
        BOOST_LOG_TRIVIAL(info)
            << "[ServerSetup] Successfully wrote default config to "
            << fullPath;
      }
    } else {
      BOOST_LOG_TRIVIAL(info)
          << "[ServerSetup] Starting with default in-memory config.";
      createDefaultCubeConfig(serverConfig);
    }
  }

  if (!serverAddressOverride.empty()) {
    if (!serverConfig.has_serverconnection()) {
      serverConfig.set_allocated_serverconnection(
          new matrixserver::Connection());
    }
    serverConfig.mutable_serverconnection()->set_serveraddress(
        serverAddressOverride);
    BOOST_LOG_TRIVIAL(info) << "[ServerSetup] Server address overridden to: "
                            << serverAddressOverride;
  }
}

} // namespace ServerSetup
