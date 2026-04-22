#ifndef MATRIXAPPLICATION_UI_ACTIONMENUITEM_H
#define MATRIXAPPLICATION_UI_ACTIONMENUITEM_H

#include <functional>

#include "MenuItem.h"

class ActionMenuItem : public MenuItem {
public:
    ActionMenuItem(std::string label,
                   std::function<void()> onActivate,
                   Color color = Color::white());

    void draw(CubeApplication &app,
              ScreenNumber screen,
              Eigen::Vector2i pos,
              bool selected,
              Color accent) const override;
    bool onActivate() override;

private:
    std::function<void()> onActivate_;
};

#endif
