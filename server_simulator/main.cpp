#include <boost/log/trivial.hpp>
#include <atomic>
#include <csignal>
#include <unistd.h>

static std::atomic<bool> g_shutdown{false};
static void signalHandler(int) { g_shutdown = true; }

#include <Server.h>
#include <ServerSetup.h>
#include <WebSocketSimulatorRenderer.h>

int main(int argc, char **argv) {
  std::signal(SIGTERM, signalHandler);
  std::signal(SIGINT,  signalHandler);

  matrixserver::ServerConfig serverConfig;
  ServerSetup::handleServerConfig(argc, argv, serverConfig);

  BOOST_LOG_TRIVIAL(info) << "ServerConfig: " << std::endl
                          << serverConfig.DebugString() << std::endl;

  auto screens = ServerSetup::createScreensFromConfig(serverConfig);

  std::string simPort = serverConfig.simulatorport();
  if (simPort.empty())
    simPort = "1337";

  auto renderer =
      std::make_shared<WebSocketSimulatorRenderer>(screens, simPort);

  Server server(renderer, serverConfig);

  int tickMs = serverConfig.tickintervalms();
  if (tickMs <= 0) tickMs = 1000;

  while (!g_shutdown && server.tick())
    usleep(tickMs * 1000);

  server.stopDefaultApp();
  return 0;
}
