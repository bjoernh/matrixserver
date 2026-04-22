#include "ActionMenuItem.h"

#include "Font6px.h"

ActionMenuItem::ActionMenuItem(std::string label,
                               std::function<void()> onActivate,
                               Color color)
    : MenuItem(std::move(label), color), onActivate_(std::move(onActivate)) {}

void ActionMenuItem::draw(CubeApplication &app,
                          ScreenNumber screen,
                          Eigen::Vector2i pos,
                          bool selected,
                          Color accent) const {
    Color textColor = selected ? accent : color_;
    app.drawText(screen, Eigen::Vector2i(CharacterBitmaps::centered, pos.y()), textColor, label_);
}

bool ActionMenuItem::onActivate() {
    if (onActivate_) {
        onActivate_();
        return true;
    }
    return false;
}
