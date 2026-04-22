#ifndef MATRIXAPPLICATION_UI_STATUSBAR_H
#define MATRIXAPPLICATION_UI_STATUSBAR_H

#include <functional>
#include <string>

#include "Color.h"

class CubeApplication;

class StatusBar {
public:
    StatusBar();
    explicit StatusBar(std::string hostname,
                       std::function<float()> voltageProvider = nullptr,
                       Color textColor = Color::blue());

    static std::string fetchHostname();

    void setHostname(std::string hostname) { hostname_ = std::move(hostname); }
    void setVoltageProvider(std::function<float()> provider) { voltageProvider_ = std::move(provider); }
    void setTextColor(Color c) { textColor_ = c; }

    void draw(CubeApplication &app) const;

private:
    std::string hostname_;
    std::function<float()> voltageProvider_;
    Color textColor_ = Color::blue();
};

#endif
