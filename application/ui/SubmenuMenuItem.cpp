#include "SubmenuMenuItem.h"

#include "Font6px.h"
#include "MenuNavigator.h"

SubmenuMenuItem::SubmenuMenuItem(std::string label, Menu *target, Color color)
    : MenuItem(std::move(label), color), target_(target) {}

void SubmenuMenuItem::draw(CubeApplication &app,
                           ScreenNumber screen,
                           Eigen::Vector2i pos,
                           bool selected,
                           Color accent) const {
    Color textColor = selected ? accent : color_;
    app.drawText(screen, Eigen::Vector2i(CharacterBitmaps::centered, pos.y()), textColor, label_);

    // 3-pixel right-chevron at the right edge of the row.
    int cx = CUBEMAXINDEX - 2;
    int cy = pos.y() + 1;
    app.drawLine2D(screen, cx - 2, cy,     cx,     cy + 2, textColor);
    app.drawLine2D(screen, cx,     cy + 2, cx - 2, cy + 4, textColor);
}

bool SubmenuMenuItem::onActivate() {
    if (nav_ && target_) {
        nav_->push(target_);
        return true;
    }
    return false;
}
