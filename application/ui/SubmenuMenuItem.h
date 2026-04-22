#ifndef MATRIXAPPLICATION_UI_SUBMENUMENUITEM_H
#define MATRIXAPPLICATION_UI_SUBMENUMENUITEM_H

#include "MenuItem.h"

class Menu;
class MenuNavigator;

class SubmenuMenuItem : public MenuItem {
public:
    SubmenuMenuItem(std::string label, Menu *target, Color color = Color::white());

    void setNavigator(MenuNavigator *nav) { nav_ = nav; }
    Menu *target() const { return target_; }

    void draw(CubeApplication &app,
              ScreenNumber screen,
              Eigen::Vector2i pos,
              bool selected,
              Color accent) const override;
    bool onActivate() override;

private:
    Menu *target_ = nullptr;
    MenuNavigator *nav_ = nullptr;
};

#endif
