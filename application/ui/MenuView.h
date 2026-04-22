#ifndef MATRIXAPPLICATION_UI_MENUVIEW_H
#define MATRIXAPPLICATION_UI_MENUVIEW_H

#include <string>

#include "Color.h"
#include "MenuInputMapping.h"

class CubeApplication;
class JoystickManager;

class MenuView {
public:
    virtual ~MenuView() = default;

    virtual void handleInput(JoystickManager &joy, const MenuInputMapping &mapping) = 0;
    virtual void update() = 0;
    virtual void draw(CubeApplication &app, Color accent) = 0;
    virtual const std::string &title() const = 0;
};

#endif
