#ifndef SNAKE_PIXELFLOW_H
#define SNAKE_PIXELFLOW_H

#include "CubeApplication.h"

#include "Joystick.h"
#ifdef BUILD_RASPBERRYPI
#include "ADS1000.h"
#endif

#include <filesystem>

namespace fs = std::filesystem;

class MainMenu : public CubeApplication {
  public:
    MainMenu();

    bool loop();

  private:
    class AppListItem;

    JoystickManager joystickmngr;
    std::vector<AppListItem> appList;
    std::vector<AppListItem> settingsList;
    std::string searchDirectory;
#ifdef BUILD_RASPBERRYPI
    ADS1000 adcBattery;
#endif
};

class MainMenu::AppListItem {
  public:
    AppListItem(std::string setName, std::string setExecPath, bool setInternal = false, Color setColor = Color::white());

    std::string name;
    std::string execPath;
    bool isInternal = false;
    Color color = Color::white();
};

#endif // SNAKE_PIXELFLOW_H
