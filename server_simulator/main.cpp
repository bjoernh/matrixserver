#include <boost/log/trivial.hpp>
#include <fstream>
#include <iostream>
#include <vector>

#include <Screen.h>
#include <Server.h>
#include <SimulatorRenderer.h>
#include <TcpServer.h>
#include <WebSocketSimulatorRenderer.h>

#include <google/protobuf/util/json_util.h>
#include <matrixserver.pb.h>

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
  serverConfig.set_simulatoraddress("127.0.0.1");
  serverConfig.set_simulatorport("1337");
}

void handleServerConfig(int argc, char **argv,
                        matrixserver::ServerConfig &serverConfig) {
  std::string serverAddressOverride = "";
  std::string configPath = "";

  if (argc >= 2) {
    serverAddressOverride = argv[1];
    BOOST_LOG_TRIVIAL(info) << "[Server] Using CLI server address override: "
                            << serverAddressOverride;
  }

  if (argc >= 3) {
    configPath = argv[2];
  }

  if (!configPath.empty()) {
    BOOST_LOG_TRIVIAL(debug)
        << "[Server] Trying to read config from: " << configPath;
    std::ifstream configFileReadStream(configPath);
    std::stringstream buffer;
    buffer << configFileReadStream.rdbuf();
    if (google::protobuf::util::JsonStringToMessage(buffer.str(), &serverConfig)
            .ok()) {
      BOOST_LOG_TRIVIAL(debug)
          << "[Server] ServerConfig successfully read from: " << configPath;
    } else {
      BOOST_LOG_TRIVIAL(debug)
          << "[Server] ServerConfig read failed from: " << configPath;
    }
  } else {
    BOOST_LOG_TRIVIAL(debug) << "[Server] creating default config";
    createDefaultCubeConfig(serverConfig);
    std::string configString;
    google::protobuf::util::JsonOptions jsonOptions;
    jsonOptions.add_whitespace = true;
    jsonOptions.always_print_primitive_fields = true;
    if (google::protobuf::util::MessageToJsonString(serverConfig, &configString,
                                                    jsonOptions)
            .ok()) {
      std::string configFileName = "matrixServerConfig.json";
      std::ofstream configFileWriteStream(configFileName, std::ios_base::trunc);
      configFileWriteStream << configString;
      configFileWriteStream.close();
      BOOST_LOG_TRIVIAL(debug)
          << "[Server] written default config to " << configFileName;
    }
  }

  if (!serverAddressOverride.empty()) {
    if (!serverConfig.has_serverconnection()) {
      serverConfig.set_allocated_serverconnection(
          new matrixserver::Connection());
    }
    serverConfig.mutable_serverconnection()->set_serveraddress(
        serverAddressOverride);
  }
}

int main(int argc, char **argv) {
  matrixserver::ServerConfig serverConfig;
  handleServerConfig(argc, argv, serverConfig);

  BOOST_LOG_TRIVIAL(info) << "ServerConfig: " << std::endl
                          << serverConfig.DebugString() << std::endl;

  std::vector<std::shared_ptr<Screen>> screens;
  for (auto screenInfo : serverConfig.screeninfo())
    screens.push_back(std::make_shared<Screen>(
        screenInfo.width(), screenInfo.height(), screenInfo.screenid()));

  bool useDeprecatedTcp = false;
  for (int i = 1; i < argc; ++i) {
    if (std::string(argv[i]) == "--use-deprecated-tcp-connection") {
      useDeprecatedTcp = true;
      break;
    }
  }

  std::shared_ptr<IRenderer> renderer;
  if (useDeprecatedTcp) {
    BOOST_LOG_TRIVIAL(info)
        << "[Server] Initialising legacy TCP SimulatorRenderer";
    renderer = std::make_shared<SimulatorRenderer>(
        screens, serverConfig.simulatoraddress(), serverConfig.simulatorport());
  } else {
    BOOST_LOG_TRIVIAL(info)
        << "[Server] Initialising WebSocketSimulatorRenderer";
    renderer = std::make_shared<WebSocketSimulatorRenderer>(
        screens, serverConfig.simulatorport());
  }

  Server server(renderer, serverConfig);

  while (server.tick())
    sleep(1);

  return 0;
}