#include <boost/log/trivial.hpp>
#include <vector>

#include <Screen.h>
#include <Server.h>
#include <ServerSetup.h>
#include <SimulatorRenderer.h>
#include <TcpServer.h>
#include <WebSocketSimulatorRenderer.h>

#include <matrixserver.pb.h>

int main(int argc, char **argv) {
  matrixserver::ServerConfig serverConfig;
  bool useDeprecatedTcp = false;
  ServerSetup::handleServerConfig(argc, argv, serverConfig, useDeprecatedTcp);

  BOOST_LOG_TRIVIAL(info) << "ServerConfig: " << std::endl
                          << serverConfig.DebugString() << std::endl;

  std::vector<std::shared_ptr<Screen>> screens;
  for (auto screenInfo : serverConfig.screeninfo())
    screens.push_back(std::make_shared<Screen>(
        screenInfo.width(), screenInfo.height(), screenInfo.screenid()));

  std::shared_ptr<IRenderer> renderer;
  std::string simPort = serverConfig.simulatorport();
  if (simPort.empty()) {
      simPort = "1337";
  }

  if (useDeprecatedTcp) {
    BOOST_LOG_TRIVIAL(info)
        << "[Server] Initialising legacy TCP SimulatorRenderer";
    renderer = std::make_shared<SimulatorRenderer>(
        screens, serverConfig.simulatoraddress(), simPort);
  } else {
    BOOST_LOG_TRIVIAL(info)
        << "[Server] Initialising WebSocketSimulatorRenderer";
    renderer = std::make_shared<WebSocketSimulatorRenderer>(
        screens, simPort);
  }

  Server server(renderer, serverConfig);

  while (server.tick())
    sleep(1);

  return 0;
}