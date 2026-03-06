#include <boost/log/trivial.hpp>
#include <iostream>
#include <memory>
#include <vector>

#include <FPGARendererRPISPI.h>
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
      screen->setOffsetX(1);
      screen->setOffsetY(1);
      screen->setRotation(Rotation::rot0);
      break;
    case matrixserver::ScreenInfo_ScreenOrientation::
        ScreenInfo_ScreenOrientation_right:
      screen->setOffsetX(2);
      screen->setOffsetY(1);
      screen->setRotation(Rotation::rot0);
      break;
    case matrixserver::ScreenInfo_ScreenOrientation::
        ScreenInfo_ScreenOrientation_back:
      screen->setOffsetX(1);
      screen->setOffsetY(0);
      screen->setRotation(Rotation::rot90);
      break;
    case matrixserver::ScreenInfo_ScreenOrientation::
        ScreenInfo_ScreenOrientation_left:
      screen->setOffsetX(0);
      screen->setOffsetY(1);
      screen->setRotation(Rotation::rot0);
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

  auto rendererFPGA = std::make_shared<FPGARendererRPISPI>(screens);

  Server server(rendererFPGA, serverConfig);

  while (server.tick()) {
    usleep(100000);
  };
  return 0;
}