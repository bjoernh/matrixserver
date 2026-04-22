#ifndef MATRIXSERVER_MAINMENU_H
#define MATRIXSERVER_MAINMENU_H

#include <string>

#include "CubeApplication.h"
#include "Joystick.h"
#include "ui/MenuUI.h"

#ifdef BUILD_RASPBERRYPI
#include "ADS1000.h"
#endif

class MainMenu : public CubeApplication {
public:
    MainMenu();

    bool loop() override;

private:
    void buildMenus();
    void discoverApps(Menu *root);
    Menu *buildSettingsMenu();
    void launchApp(const std::string &execPath);
    void drawRootBranding(Color accent);
    void drawUpdateScreen(Color accent);

    JoystickManager joystickmngr_;
    MenuNavigator navigator_;
    StatusBar statusBar_;
    Menu *rootMenu_ = nullptr;
    Color accentColor_;
    int loopCount_ = 0;
    bool updateInProgress_ = false;
    bool demoMode_ = false;
    std::string searchDirectory_;
#ifdef BUILD_RASPBERRYPI
    ADS1000 adcBattery_;
#endif
};

#endif
