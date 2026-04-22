#ifndef MATRIXAPPLICATION_UI_MENUITEM_H
#define MATRIXAPPLICATION_UI_MENUITEM_H

#include <string>
#include <Eigen/Dense>

#include "Color.h"
#include "CubeApplication.h"

class MenuItem {
public:
    MenuItem(std::string label, Color color = Color::white());
    virtual ~MenuItem() = default;

    virtual void draw(CubeApplication &app,
                      ScreenNumber screen,
                      Eigen::Vector2i pos,
                      bool selected,
                      Color accent) const = 0;
    virtual bool onActivate() { return false; }
    virtual void onAxisChange(float dx) { (void)dx; }
    virtual bool isSelectable() const { return enabled_; }

    const std::string &label() const { return label_; }
    void setLabel(std::string label) { label_ = std::move(label); }
    Color color() const { return color_; }
    void setColor(Color color) { color_ = color; }
    void setEnabled(bool enabled) { enabled_ = enabled; }

protected:
    std::string label_;
    Color color_;
    bool enabled_ = true;
};

#endif
