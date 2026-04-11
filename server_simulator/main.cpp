#include <boost/log/trivial.hpp>
#include <chrono>
#include <thread>
#include <vector>

#include <Screen.h>
#include <Server.h>
#include <ServerSetup.h>
#include <SimulatorRenderer.h>
#include <TcpServer.h>

#ifndef _WIN32
#include <WebSocketSimulatorRenderer.h>
#endif

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

#ifdef _WIN32
  // WebSocket renderer requires boost-beast (not available); use legacy TCP
  useDeprecatedTcp = true;
#endif

  if (useDeprecatedTcp) {
    BOOST_LOG_TRIVIAL(info)
        << "[Server] Initialising legacy TCP SimulatorRenderer";
    renderer = std::make_shared<SimulatorRenderer>(
        screens, serverConfig.simulatoraddress(), simPort);
  } else {
#ifndef _WIN32
    BOOST_LOG_TRIVIAL(info)
        << "[Server] Initialising WebSocketSimulatorRenderer";
    renderer = std::make_shared<WebSocketSimulatorRenderer>(
        screens, simPort);
#endif
  }

  Server server(renderer, serverConfig);

  while (server.tick())
    std::this_thread::sleep_for(std::chrono::seconds(1));

  return 0;
}
