#include <boost/log/trivial.hpp>
#include <iostream>
#include <memory>
#include <vector>

#include <FPGARendererFTDI.h>
#include <Server.h>
#include <ServerSetup.h>
#include <TcpServer.h>

#include <matrixserver.pb.h>

int main(int argc, char **argv) {
  matrixserver::ServerConfig serverConfig;
  bool ignoredUseDeprecatedTcp;
  ServerSetup::handleServerConfig(argc, argv, serverConfig,
                                  ignoredUseDeprecatedTcp);

  BOOST_LOG_TRIVIAL(info) << "ServerConfig: " << std::endl
                          << serverConfig.DebugString() << std::endl;

  std::vector<std::shared_ptr<Screen>> screens;
  for (auto screenInfo : serverConfig.screeninfo()) {
    auto screen = std::make_shared<Screen>(
        screenInfo.width(), screenInfo.height(), screenInfo.screenid());
    switch (screenInfo.screenorientation()) {
    case matrixserver::ScreenInfo_ScreenOrientation::
        ScreenInfo_ScreenOrientation_front:
      screen->setOffsetX(4);
      screen->setOffsetY(0);
      screen->setRotation(Rotation::rot180);
      break;
    case matrixserver::ScreenInfo_ScreenOrientation::
        ScreenInfo_ScreenOrientation_right:
      screen->setOffsetX(3);
      screen->setOffsetY(0);
      screen->setRotation(Rotation::rot180);
      break;
    case matrixserver::ScreenInfo_ScreenOrientation::
        ScreenInfo_ScreenOrientation_back:
      screen->setOffsetX(1);
      screen->setOffsetY(0);
      screen->setRotation(Rotation::rot90);
      break;
    case matrixserver::ScreenInfo_ScreenOrientation::
        ScreenInfo_ScreenOrientation_left:
      screen->setOffsetX(5);
      screen->setOffsetY(0);
      screen->setRotation(Rotation::rot180);
      break;
    case matrixserver::ScreenInfo_ScreenOrientation::
        ScreenInfo_ScreenOrientation_top:
      screen->setOffsetX(0);
      screen->setOffsetY(0);
      screen->setRotation(Rotation::rot270);
      break;
    case matrixserver::ScreenInfo_ScreenOrientation::
        ScreenInfo_ScreenOrientation_bottom:
      screen->setOffsetX(2);
      screen->setOffsetY(0);
      screen->setRotation(Rotation::rot270);
      break;
    default:
      break;
    }
    screens.push_back(screen);
  }

  auto rendererFPGA = std::make_shared<FPGARendererFTDI>(screens);

  Server server(rendererFPGA, serverConfig);

  while (server.tick()) {
    sleep(1);
  };
  return 0;
}