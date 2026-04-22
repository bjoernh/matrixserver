#include "CheckboxMenuItem.h"

#include "Font6px.h"

CheckboxMenuItem::CheckboxMenuItem(std::string label,
                                   std::function<bool()> getter,
                                   std::function<void(bool)> setter,
                                   Color color)
    : MenuItem(std::move(label), color),
      getter_(std::move(getter)),
      setter_(std::move(setter)) {}

void CheckboxMenuItem::draw(CubeApplication &app,
                            ScreenNumber screen,
                            Eigen::Vector2i pos,
                            bool selected,
                            Color accent) const {
    Color textColor = selected ? accent : color_;
    app.drawText(screen, Eigen::Vector2i(CharacterBitmaps::centered, pos.y()), textColor, label_);

    // 5x5 outlined box on the right of the row.
    int x0 = CUBEMAXINDEX - 5;
    int y0 = pos.y();
    int x1 = x0 + 4;
    int y1 = y0 + 4;
    app.drawRect2D(screen, x0, y0, x1, y1, textColor);

    if (getter_ && getter_()) {
        // X-mark inside the box.
        app.drawLine2D(screen, x0 + 1, y0 + 1, x1 - 1, y1 - 1, textColor);
        app.drawLine2D(screen, x1 - 1, y0 + 1, x0 + 1, y1 - 1, textColor);
    }
}

bool CheckboxMenuItem::onActivate() {
    if (getter_ && setter_) {
        setter_(!getter_());
        return true;
    }
    return false;
}
