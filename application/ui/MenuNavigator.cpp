#include "MenuNavigator.h"

#include "BackMenuItem.h"
#include "CubeApplication.h"
#include "Joystick.h"
#include "SubmenuMenuItem.h"

MenuNavigator::MenuNavigator() = default;

void MenuNavigator::wireMenuItems(Menu &menu) {
    for (std::size_t i = 0; i < menu.itemCount(); ++i) {
        MenuItem *raw = menu.itemAt(i);
        if (auto *sub = dynamic_cast<SubmenuMenuItem *>(raw)) {
            sub->setNavigator(this);
        } else if (auto *back = dynamic_cast<BackMenuItem *>(raw)) {
            back->setNavigator(this);
        }
    }
}

Menu *MenuNavigator::addMenu(std::unique_ptr<Menu> menu) {
    if (!menu) return nullptr;
    Menu *raw = menu.get();
    ownedMenus_.push_back(std::move(menu));
    wireMenuItems(*raw);
    return raw;
}

void MenuNavigator::setRootMenu(Menu *root) {
    rootMenu_ = root;
    stack_.clear();
    if (root) stack_.push_back(root);
}

void MenuNavigator::push(MenuView *view) {
    if (!view) return;
    stack_.push_back(view);
}

void MenuNavigator::pop() {
    if (stack_.size() > 1) {
        stack_.pop_back();
    }
}

MenuView *MenuNavigator::current() const {
    if (stack_.empty()) return nullptr;
    return stack_.back();
}

void MenuNavigator::rewireAll() {
    for (auto &m : ownedMenus_) {
        wireMenuItems(*m);
    }
}

void MenuNavigator::update(JoystickManager &joy) {
    MenuView *view = current();
    if (!view) return;
    view->update();

    if (stack_.size() > 1 && joy.getButtonPress(mapping_.backButton)) {
        pop();
        return;
    }
    view->handleInput(joy, mapping_);
}

void MenuNavigator::draw(CubeApplication &app) {
    MenuView *view = current();
    if (!view) return;
    view->draw(app, accentColor_);
}
