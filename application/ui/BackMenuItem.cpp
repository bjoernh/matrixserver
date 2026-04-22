#include "BackMenuItem.h"

#include "Font6px.h"
#include "MenuNavigator.h"

BackMenuItem::BackMenuItem(std::string label, Color color)
    : MenuItem(std::move(label), color) {}

void BackMenuItem::draw(CubeApplication &app,
                        ScreenNumber screen,
                        Eigen::Vector2i pos,
                        bool selected,
                        Color accent) const {
    Color textColor = selected ? accent : color_;
    app.drawText(screen, Eigen::Vector2i(CharacterBitmaps::centered, pos.y()), textColor, label_);
}

bool BackMenuItem::onActivate() {
    if (nav_) {
        nav_->pop();
        return true;
    }
    return false;
}
