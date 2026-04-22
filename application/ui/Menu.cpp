#include "Menu.h"

#include "Joystick.h"

Menu::Menu(std::string title, Color titleColor)
    : title_(std::move(title)), titleColor_(titleColor) {}

MenuItem *Menu::addItem(std::unique_ptr<MenuItem> item) {
    items_.push_back(std::move(item));
    return items_.back().get();
}

MenuItem *Menu::itemAt(std::size_t index) const {
    if (index >= items_.size()) return nullptr;
    return items_[index].get();
}

void Menu::setSelectedIndex(int index) {
    if (items_.empty()) {
        selectedIndex_ = 0;
        lastSelectedIndex_ = 0;
        return;
    }
    if (index < 0) index = 0;
    if (index >= static_cast<int>(items_.size())) index = items_.size() - 1;
    selectedIndex_ = index;
    lastSelectedIndex_ = index;
    animationOffset_ = 0.0f;
}

void Menu::selectFirstEnabled() {
    for (std::size_t i = 0; i < items_.size(); ++i) {
        if (items_[i]->isSelectable()) {
            setSelectedIndex(static_cast<int>(i));
            return;
        }
    }
    setSelectedIndex(0);
}

void Menu::moveSelection(int delta) {
    if (items_.empty() || delta == 0) return;
    int n = static_cast<int>(items_.size());
    int next = selectedIndex_;
    // Step one by one so we can skip disabled items; bail out if nothing is selectable.
    for (int tries = 0; tries < n; ++tries) {
        next = (next + delta % n + n) % n;
        if (items_[next]->isSelectable()) {
            selectedIndex_ = next;
            return;
        }
    }
}

void Menu::handleInput(JoystickManager &joy, const MenuInputMapping &mapping) {
    int axisStep = static_cast<int>(joy.getAxisPress(mapping.axisVertical));
    if (axisStep != 0) {
        moveSelection(axisStep);
    }

    float horizontalDelta = joy.getAxisPress(mapping.axisHorizontal);
    if (!items_.empty() && selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(items_.size())) {
        items_[selectedIndex_]->onAxisChange(horizontalDelta);
    }

    if (joy.getButtonPress(mapping.activateButton) && !items_.empty()) {
        items_[selectedIndex_]->onActivate();
    }
}

void Menu::update() {
    if (lastSelectedIndex_ != selectedIndex_) {
        animationOffset_ = static_cast<float>(selectedIndex_ - lastSelectedIndex_) * rowHeight_;
        lastSelectedIndex_ = selectedIndex_;
    }
    animationOffset_ *= 0.85f;
    if (animationOffset_ > -0.5f && animationOffset_ < 0.5f) animationOffset_ = 0.0f;
}

void Menu::draw(CubeApplication &app, Color accent) {
    for (std::size_t i = 0; i < items_.size(); ++i) {
        int yPos = centerY_ +
                   (static_cast<int>(i) - selectedIndex_) * rowHeight_ +
                   static_cast<int>(animationOffset_);
        if (yPos <= 6 || yPos >= 57) continue;
        bool selected = (static_cast<int>(i) == selectedIndex_);
        for (int screenCounter = 0; screenCounter < 4; ++screenCounter) {
            items_[i]->draw(app,
                            static_cast<ScreenNumber>(screenCounter),
                            Eigen::Vector2i(0, yPos),
                            selected,
                            accent);
        }
    }

    if (!title_.empty()) {
        app.drawText(top,
                     Eigen::Vector2i(CharacterBitmaps::centered, 30),
                     titleColor_,
                     title_);
    }
}
