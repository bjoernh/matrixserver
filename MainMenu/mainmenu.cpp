#include "mainmenu.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#include <boost/log/trivial.hpp>

#include "Font6px.h"

namespace {

bool isExecutableFile(const std::filesystem::path &path) {
    std::error_code ec;
    if (!std::filesystem::is_regular_file(path, ec)) return false;
    return access(path.c_str(), X_OK) == 0;
}

std::vector<std::filesystem::path> collectExecutables(const std::filesystem::path &dir) {
    std::vector<std::filesystem::path> out;
    std::error_code ec;
    if (!std::filesystem::is_directory(dir, ec)) return out;
    for (const auto &entry : std::filesystem::directory_iterator(dir, ec)) {
        if (isExecutableFile(entry.path())) {
            out.push_back(entry.path());
        }
    }
    std::sort(out.begin(), out.end());
    return out;
}

}  // namespace

MainMenu::MainMenu() : CubeApplication(40), joystickmngr_(8) {
    const char *appPath = std::getenv("CUBE_APP_PATH");
    if (appPath) {
        searchDirectory_ = std::string(appPath);
    } else {
        const char *homeDir = std::getenv("HOME");
        searchDirectory_ = std::string(homeDir ? homeDir : "/home/pi") + "/APPS";
    }

    statusBar_.setHostname(StatusBar::fetchHostname());
#ifdef BUILD_RASPBERRYPI
    statusBar_.setVoltageProvider([this]() { return adcBattery_.getVoltage(); });
#endif

    buildMenus();

    // Apply brightness from the matrixserver config on startup.
    setBrightness(getBrightness());
}

void MainMenu::buildMenus() {
    auto root = std::make_unique<Menu>();
    rootMenu_ = navigator_.addMenu(std::move(root));
    navigator_.setRootMenu(rootMenu_);

    discoverApps(rootMenu_);

    Menu *settings = buildSettingsMenu();
    rootMenu_->addItem(std::make_unique<SubmenuMenuItem>("settings", settings, Color::blue()));

    // Items were added after addMenu() registration; patch in navigator pointers now.
    navigator_.rewireAll();
}

void MainMenu::discoverApps(Menu *root) {
    std::error_code ec;
    if (!std::filesystem::is_directory(searchDirectory_, ec)) {
        BOOST_LOG_TRIVIAL(warning) << "[MainMenu] Apps directory not found: " << searchDirectory_;
        return;
    }

    struct Category {
        std::string name;
        std::vector<std::filesystem::path> apps;
    };

    std::vector<Category> categories;
    std::vector<std::filesystem::path> looseApps;

    for (const auto &entry : std::filesystem::directory_iterator(searchDirectory_, ec)) {
        if (std::filesystem::is_directory(entry.path(), ec)) {
            auto apps = collectExecutables(entry.path());
            if (!apps.empty()) {
                categories.push_back({entry.path().filename().string(), std::move(apps)});
            }
        } else if (isExecutableFile(entry.path())) {
            looseApps.push_back(entry.path());
        }
    }

    std::sort(looseApps.begin(), looseApps.end());

    // Preferred-order-first, then remaining categories alphabetically.
    const std::vector<std::string> preferredOrder = {"Animation", "Demos", "Games"};
    std::vector<Category> ordered;
    for (const auto &name : preferredOrder) {
        auto it = std::find_if(categories.begin(), categories.end(),
                               [&](const Category &c) { return c.name == name; });
        if (it != categories.end()) {
            ordered.push_back(std::move(*it));
            categories.erase(it);
        }
    }
    std::sort(categories.begin(), categories.end(),
              [](const Category &a, const Category &b) { return a.name < b.name; });
    for (auto &c : categories) ordered.push_back(std::move(c));

    // If there are no categories at all, put loose apps directly on root
    // (backward-compat with the old flat layout).
    bool flatLayout = ordered.empty();

    for (auto &cat : ordered) {
        auto submenu = std::make_unique<Menu>(cat.name);
        Menu *submenuPtr = navigator_.addMenu(std::move(submenu));
        for (auto &path : cat.apps) {
            std::string execPath = path.string();
            std::string name = path.filename().string();
            submenuPtr->addItem(std::make_unique<ActionMenuItem>(
                name, [this, execPath]() { launchApp(execPath); }));
        }
        submenuPtr->addItem(std::make_unique<BackMenuItem>());
        root->addItem(std::make_unique<SubmenuMenuItem>(cat.name, submenuPtr));
    }

    if (!looseApps.empty()) {
        if (flatLayout) {
            for (auto &path : looseApps) {
                std::string execPath = path.string();
                std::string name = path.filename().string();
                root->addItem(std::make_unique<ActionMenuItem>(
                    name, [this, execPath]() { launchApp(execPath); }));
            }
        } else {
            auto appsMenu = std::make_unique<Menu>("Apps");
            Menu *appsPtr = navigator_.addMenu(std::move(appsMenu));
            for (auto &path : looseApps) {
                std::string execPath = path.string();
                std::string name = path.filename().string();
                appsPtr->addItem(std::make_unique<ActionMenuItem>(
                    name, [this, execPath]() { launchApp(execPath); }));
            }
            appsPtr->addItem(std::make_unique<BackMenuItem>());
            root->addItem(std::make_unique<SubmenuMenuItem>("Apps", appsPtr));
        }
    }
}

Menu *MainMenu::buildSettingsMenu() {
    auto settings = std::make_unique<Menu>("Settings");
    Menu *ptr = navigator_.addMenu(std::move(settings));

    ptr->addItem(std::make_unique<IntSliderMenuItem>(
        "brightness",
        0, 100, 5,
        [this]() { return getBrightness(); },
        [this](int v) { setBrightness(v); }));

    ptr->addItem(std::make_unique<CheckboxMenuItem>(
        "demo mode",
        [this]() { return demoMode_; },
        [this](bool v) { demoMode_ = v; }));

    ptr->addItem(std::make_unique<ActionMenuItem>(
        "update",
        [this]() {
            updateInProgress_ = true;
            (void)std::system("nohup /usr/local/sbin/Update.sh 1>/dev/null 2>/dev/null &");
        }));

    ptr->addItem(std::make_unique<ActionMenuItem>(
        "shutdown",
        []() { (void)std::system("sudo shutdown now"); }));

    ptr->addItem(std::make_unique<BackMenuItem>("return"));

    return ptr;
}

void MainMenu::launchApp(const std::string &execPath) {
    std::string cmd = "nohup " + execPath + " 1>/dev/null 2>/dev/null &";
    BOOST_LOG_TRIVIAL(info) << "[MainMenu] launch: " << cmd;
    (void)std::system(cmd.c_str());
}

void MainMenu::drawRootBranding(Color accent) {
    drawRect2D(top, 10, 10, 53, 53, accent);
    drawText(top, Eigen::Vector2i(CharacterBitmaps::centered, 22), accent, "DOT");
    drawText(top, Eigen::Vector2i(CharacterBitmaps::centered, 30), accent, "THE");
    drawText(top, Eigen::Vector2i(CharacterBitmaps::centered, 38), accent, "LEDCUBE");
}

void MainMenu::drawUpdateScreen(Color accent) {
    for (int screenCounter = 0; screenCounter < 5; ++screenCounter) {
        auto screen = static_cast<ScreenNumber>(screenCounter);
        drawText(screen, Eigen::Vector2i(CharacterBitmaps::centered, 22), accent, "Update");
        drawText(screen, Eigen::Vector2i(CharacterBitmaps::centered, 30), accent, "in");
        drawText(screen, Eigen::Vector2i(CharacterBitmaps::centered, 38), accent, "progress");
    }
}

bool MainMenu::loop() {
    clear();
    accentColor_.fromHSV(static_cast<float>(loopCount_ % 360), 1.0f, 1.0f);
    navigator_.setAccentColor(accentColor_);

    if (updateInProgress_) {
        drawUpdateScreen(accentColor_);
    } else {
        navigator_.update(joystickmngr_);
        navigator_.draw(*this);
        if (navigator_.current() == rootMenu_) {
            drawRootBranding(accentColor_);
        }
    }

    statusBar_.draw(*this);

    joystickmngr_.clearAllButtonPresses();
    render();
    ++loopCount_;
    return true;
}
