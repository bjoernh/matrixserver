#ifndef MATRIXAPPLICATION_UI_SLIDERMENUITEM_H
#define MATRIXAPPLICATION_UI_SLIDERMENUITEM_H

#include <algorithm>
#include <functional>
#include <string>
#include <type_traits>

#include "Font6px.h"
#include "MenuItem.h"

template <typename T>
class SliderMenuItem : public MenuItem {
    static_assert(std::is_arithmetic<T>::value, "SliderMenuItem<T> requires arithmetic T");

public:
    SliderMenuItem(std::string label,
                   T min,
                   T max,
                   T step,
                   std::function<T()> getter,
                   std::function<void(T)> setter,
                   Color color = Color::white(),
                   bool showValueInLabel = true)
        : MenuItem(std::move(label), color),
          min_(min),
          max_(max),
          step_(step),
          getter_(std::move(getter)),
          setter_(std::move(setter)),
          showValueInLabel_(showValueInLabel) {}

    void draw(CubeApplication &app,
              ScreenNumber screen,
              Eigen::Vector2i pos,
              bool selected,
              Color accent) const override {
        Color textColor = selected ? accent : color_;
        std::string text = label_;
        if (showValueInLabel_ && getter_) {
            text += " ";
            text += std::to_string(static_cast<long long>(getter_()));
        }
        app.drawText(screen, Eigen::Vector2i(CharacterBitmaps::centered, pos.y()), textColor, text);

        // Horizontal bar under the text.
        int barY = pos.y() + 6;
        int barX0 = 4;
        int barX1 = CUBEMAXINDEX - 4;
        int barWidth = barX1 - barX0;
        if (barWidth <= 0 || !getter_) return;

        T value = getter_();
        float ratio = 0.0f;
        if (max_ > min_) {
            ratio = static_cast<float>(value - min_) / static_cast<float>(max_ - min_);
        }
        if (ratio < 0.0f) ratio = 0.0f;
        if (ratio > 1.0f) ratio = 1.0f;
        int filledWidth = static_cast<int>(ratio * static_cast<float>(barWidth));

        // Track (unfilled) is always a dim background so the fill stands out.
        Color trackColor(30, 30, 30);
        Color fillColor = selected ? accent : color_;
        app.drawLine2D(screen, barX0, barY, barX1, barY, trackColor);
        if (filledWidth > 0) {
            app.drawLine2D(screen, barX0, barY, barX0 + filledWidth, barY, fillColor);
        }
    }

    void onAxisChange(float dx) override {
        if (dx == 0.0f || !getter_ || !setter_) return;
        T delta = static_cast<T>(dx) * step_;
        T next = getter_() + delta;
        if (next < min_) next = min_;
        if (next > max_) next = max_;
        setter_(next);
    }

    T min() const { return min_; }
    T max() const { return max_; }
    T step() const { return step_; }

private:
    T min_;
    T max_;
    T step_;
    std::function<T()> getter_;
    std::function<void(T)> setter_;
    bool showValueInLabel_;
};

using IntSliderMenuItem = SliderMenuItem<int>;
using FloatSliderMenuItem = SliderMenuItem<float>;

#endif
