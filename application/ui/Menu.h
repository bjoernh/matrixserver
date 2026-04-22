#ifndef MATRIXAPPLICATION_UI_MENU_H
#define MATRIXAPPLICATION_UI_MENU_H

#include <memory>
#include <string>
#include <vector>

#include "MenuItem.h"
#include "MenuView.h"

class Menu : public MenuView {
public:
    explicit Menu(std::string title = "", Color titleColor = Color::blue());

    MenuItem *addItem(std::unique_ptr<MenuItem> item);
    std::size_t itemCount() const { return items_.size(); }
    MenuItem *itemAt(std::size_t index) const;
    const std::vector<std::unique_ptr<MenuItem>> &items() const { return items_; }

    int selectedIndex() const { return selectedIndex_; }
    void setSelectedIndex(int index);
    void selectFirstEnabled();

    Color titleColor() const { return titleColor_; }
    void setTitleColor(Color color) { titleColor_ = color; }

    // MenuView interface.
    void handleInput(JoystickManager &joy, const MenuInputMapping &mapping) override;
    void update() override;
    void draw(CubeApplication &app, Color accent) override;
    const std::string &title() const override { return title_; }

private:
    void moveSelection(int delta);

    std::string title_;
    Color titleColor_;
    std::vector<std::unique_ptr<MenuItem>> items_;
    int selectedIndex_ = 0;
    int lastSelectedIndex_ = 0;
    float animationOffset_ = 0.0f;
    static constexpr int rowHeight_ = 7;
    static constexpr int centerY_ = 29;
};

#endif
