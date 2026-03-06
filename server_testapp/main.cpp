#include <boost/log/trivial.hpp>
#include <iostream>
#include <memory>
#include <vector>

#include <Screen.h>
#include <Server.h>
#include <ServerSetup.h>
#include <TcpServer.h>
#include <TestRenderer.h>

#include <matrixserver.pb.h>

const std::vector<cv::String> cvWindows = {"0", "1", "2", "3", "4", "5"};

int main(int argc, char **argv) {
  matrixserver::ServerConfig serverConfig;
  bool ignoredUseDeprecatedTcp;
  ServerSetup::handleServerConfig(argc, argv, serverConfig,
                                  ignoredUseDeprecatedTcp);

  BOOST_LOG_TRIVIAL(info) << "ServerConfig: " << std::endl
                          << serverConfig.DebugString() << std::endl;

  std::vector<std::shared_ptr<Screen>> screens;
  for (auto screenInfo : serverConfig.screeninfo())
    screens.push_back(std::make_shared<Screen>(
        screenInfo.width(), screenInfo.height(), screenInfo.screenid()));

  auto renderer = std::make_shared<TestRenderer>(TestRenderer(screens));

  Server server(renderer, serverConfig);

  while (server.tick() && cv::waitKey(25) != ' ') {
    for (auto screen : screens) {
      cv::Mat M(screen->getWidth(), screen->getHeight(), CV_8UC3,
                screen->getScreenData().data());
      cv::Mat dest;
      cv::resize(M, dest, cv::Size(256, 256), 0, 0, cv::INTER_NEAREST);
      imshow(cvWindows[screen->getScreenId()], dest);
    }
  };
  return 0;
}