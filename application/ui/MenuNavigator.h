#ifndef MATRIXAPPLICATION_UI_MENUNAVIGATOR_H
#define MATRIXAPPLICATION_UI_MENUNAVIGATOR_H

#include <memory>
#include <vector>

#include "Color.h"
#include "Menu.h"
#include "MenuInputMapping.h"
#include "MenuView.h"

class CubeApplication;
class JoystickManager;

class MenuNavigator {
public:
    MenuNavigator();

    // Registers a menu; the navigator takes ownership. It also walks the menu's
    // items and wires SubmenuMenuItem / BackMenuItem with a pointer back to
    // itself so onActivate() can push/pop.
    Menu *addMenu(std::unique_ptr<Menu> menu);

    void setRootMenu(Menu *root);
    void push(MenuView *view);
    void pop();
    MenuView *current() const;
    Menu *rootMenu() const { return rootMenu_; }

    void setInputMapping(const MenuInputMapping &mapping) { mapping_ = mapping; }
    const MenuInputMapping &inputMapping() const { return mapping_; }

    void setAccentColor(Color c) { accentColor_ = c; }
    Color accentColor() const { return accentColor_; }

    // Wires SubmenuMenuItem / BackMenuItem for every registered menu.
    // Call this after finishing menu construction if items were added after addMenu().
    void rewireAll();

    // Poll joystick and route to the current view (or pop on back-button).
    void update(JoystickManager &joy);

    // Render the active view.
    void draw(CubeApplication &app);

private:
    void wireMenuItems(Menu &menu);

    std::vector<std::unique_ptr<Menu>> ownedMenus_;
    std::vector<MenuView *> stack_;
    Menu *rootMenu_ = nullptr;
    MenuInputMapping mapping_;
    Color accentColor_ = Color::white();
};

#endif
