#include "MenuItem.h"

MenuItem::MenuItem(std::string label, Color color)
    : label_(std::move(label)), color_(color) {}
