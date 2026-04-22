#ifndef MATRIXAPPLICATION_UI_CHECKBOXMENUITEM_H
#define MATRIXAPPLICATION_UI_CHECKBOXMENUITEM_H

#include <functional>

#include "MenuItem.h"

class CheckboxMenuItem : public MenuItem {
public:
    CheckboxMenuItem(std::string label,
                     std::function<bool()> getter,
                     std::function<void(bool)> setter,
                     Color color = Color::white());

    void draw(CubeApplication &app,
              ScreenNumber screen,
              Eigen::Vector2i pos,
              bool selected,
              Color accent) const override;
    bool onActivate() override;

private:
    std::function<bool()> getter_;
    std::function<void(bool)> setter_;
};

#endif
