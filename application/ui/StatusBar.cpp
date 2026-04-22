#include "StatusBar.h"

#include <array>
#include <string>
#include <unistd.h>

#include "CubeApplication.h"
#include "Font6px.h"

StatusBar::StatusBar() = default;

StatusBar::StatusBar(std::string hostname,
                     std::function<float()> voltageProvider,
                     Color textColor)
    : hostname_(std::move(hostname)),
      voltageProvider_(std::move(voltageProvider)),
      textColor_(textColor) {}

std::string StatusBar::fetchHostname() {
    std::array<char, 64> buffer{};
    if (gethostname(buffer.data(), buffer.size()) != 0) {
        return {};
    }
    buffer.back() = '\0';
    return std::string(buffer.data());
}

void StatusBar::draw(CubeApplication &app) const {
    for (int screenCounter = 0; screenCounter < 4; ++screenCounter) {
        auto screen = static_cast<ScreenNumber>(screenCounter);
        app.drawRect2D(screen, 0, 0, CUBEMAXINDEX, 6,
                       Color::black(), true, Color::black());
        app.drawRect2D(screen, 0, CUBEMAXINDEX - 6, CUBEMAXINDEX, CUBEMAXINDEX,
                       Color::black(), true, Color::black());

        if (!hostname_.empty()) {
            app.drawText(screen,
                         Eigen::Vector2i(CharacterBitmaps::left, 1),
                         textColor_,
                         hostname_);
        }
        if (voltageProvider_) {
            float voltage = voltageProvider_();
            std::string text = std::to_string(voltage);
            if (text.size() > 5) text = text.substr(0, 5);
            text += " V";
            app.drawText(screen,
                         Eigen::Vector2i(CharacterBitmaps::right, 58),
                         textColor_,
                         text);
        }
    }
}
