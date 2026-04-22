#ifndef MATRIXAPPLICATION_UI_BACKMENUITEM_H
#define MATRIXAPPLICATION_UI_BACKMENUITEM_H

#include "MenuItem.h"

class MenuNavigator;

class BackMenuItem : public MenuItem {
public:
    explicit BackMenuItem(std::string label = "back", Color color = Color::blue());

    void setNavigator(MenuNavigator *nav) { nav_ = nav; }

    void draw(CubeApplication &app,
              ScreenNumber screen,
              Eigen::Vector2i pos,
              bool selected,
              Color accent) const override;
    bool onActivate() override;

private:
    MenuNavigator *nav_ = nullptr;
};

#endif
