#include "ServerSetup.h"

#include <array>
#include <boost/log/trivial.hpp>
#include <boost/program_options.hpp>
#include <exception>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

#include <google/protobuf/util/json_util.h>

namespace po = boost::program_options;

namespace ServerSetup {

namespace {

// Layout entry for one (HardwareType, ScreenOrientation) combination.
struct ScreenLayout {
    int offsetX;
    int offsetY;
    matrixserver::ScreenInfo::ScreenRotation rotation;
};

// Shorthand aliases to keep table rows concise.
using SI = matrixserver::ScreenInfo;
using Rot = SI::ScreenRotation;
constexpr Rot R0 = SI::rot0;
constexpr Rot R90 = SI::rot90;
constexpr Rot R180 = SI::rot180;
constexpr Rot R270 = SI::rot270;

// kLayouts[hwType_index][orientation_index]
//
// ScreenOrientation enum: defaultScreenOrientation=0, front=1, right=2,
//   back=3, left=4, top=5, bottom=6.
// HardwareType enum order: Simulator=0, FPGA_FTDI=1, FPGA_RPISPI=2, RGB_MATRIX=3
//
// Index 0 (defaultScreenOrientation) is never queried; filled with a
// zero-sentinel so every row has the same width.
constexpr int kHwTypeCount = 4;      // Simulator, FPGA_FTDI, FPGA_RPISPI, RGB_MATRIX
constexpr int kOrientationCount = 7; // 0 (unused) + front..bottom

constexpr std::array<std::array<ScreenLayout, kOrientationCount>, kHwTypeCount> kLayouts = {{
    // Simulator — all screens at (0,0,rot0)
    {{
        {0, 0, R0}, // [0] unused
        {0, 0, R0}, // front
        {0, 0, R0}, // right
        {0, 0, R0}, // back
        {0, 0, R0}, // left
        {0, 0, R0}, // top
        {0, 0, R0}, // bottom
    }},
    // FPGA_FTDI
    {{
        {0, 0, R0},   // [0] unused
        {4, 0, R180}, // front
        {3, 0, R180}, // right
        {1, 0, R90},  // back
        {5, 0, R180}, // left
        {0, 0, R270}, // top
        {2, 0, R270}, // bottom
    }},
    // FPGA_RPISPI
    {{
        {0, 0, R0},   // [0] unused
        {1, 1, R0},   // front
        {2, 1, R0},   // right
        {1, 0, R90},  // back
        {0, 1, R0},   // left
        {0, 0, R270}, // top
        {2, 0, R270}, // bottom
    }},
    // RGB_MATRIX
    {{
        {0, 0, R0},   // [0] unused
        {0, 0, R270}, // front
        {1, 0, R180}, // right
        {2, 0, R270}, // back
        {3, 0, R180}, // left
        {4, 0, R90},  // top
        {5, 0, R0},   // bottom
    }},
}};

} // anonymous namespace

static void setScreenDefaults(matrixserver::ScreenInfo *si, matrixserver::ScreenInfo::ScreenOrientation orient, HardwareType hwType) {
    si->set_screenorientation(orient);

    const auto &layout = kLayouts[static_cast<int>(hwType)][static_cast<int>(orient)];

    si->set_offsetx(layout.offsetX);
    si->set_offsety(layout.offsetY);
    si->set_screenrotation(layout.rotation);
}

void createDefaultCubeConfig(matrixserver::ServerConfig &serverConfig, HardwareType hwType) {
    serverConfig.Clear();
    serverConfig.set_globalscreenbrightness(100);
    serverConfig.set_servername("matrixserver");
    matrixserver::Connection *serverConnection = new matrixserver::Connection();
    serverConnection->set_serveraddress("127.0.0.1");
    serverConnection->set_serverport("2017");
    serverConnection->set_connectiontype(matrixserver::Connection_ConnectionType_tcp);
    serverConfig.set_allocated_serverconnection(serverConnection);
    serverConfig.set_assemblytype(matrixserver::ServerConfig_AssemblyType_cube);

    const matrixserver::ScreenInfo::ScreenOrientation orientations[] = {matrixserver::ScreenInfo::front, matrixserver::ScreenInfo::right,
                                                                        matrixserver::ScreenInfo::back,  matrixserver::ScreenInfo::left,
                                                                        matrixserver::ScreenInfo::top,   matrixserver::ScreenInfo::bottom};

    for (int i = 0; i < 6; i++) {
        auto screenInfo = serverConfig.add_screeninfo();
        screenInfo->set_screenid(i);
        screenInfo->set_available(true);
        screenInfo->set_height(64);
        screenInfo->set_width(64);
        setScreenDefaults(screenInfo, orientations[i], hwType);
    }

    serverConfig.set_simulatoraddress("127.0.0.1");
    serverConfig.set_simulatorport("1337");

    // Default tick interval: 100ms for FPGA_RPISPI, 1000ms for others
    int tickMs = (hwType == HardwareType::FPGA_RPISPI) ? 100 : 1000;
    serverConfig.set_tickintervalms(tickMs);

    // Pixel streaming is only enabled for the simulator
    serverConfig.set_pixelstreamingenabled(hwType == HardwareType::Simulator);
}

static void warnIfConfigSuspicious(const matrixserver::ServerConfig &serverConfig) {
    // Orientations that legitimately map to offsetX=0,offsetY=0,rot0 are only
    // valid for the very first panel. Any other orientation at (0,0,rot0) is a
    // sign that the layout fields were never written (old config format).
    const auto defaultOrientation = matrixserver::ScreenInfo::defaultScreenOrientation;
    const auto defaultRotation = matrixserver::ScreenInfo::rot0;

    // Track (offsetX, offsetY) usage to detect duplicates.
    std::map<std::pair<int, int>, std::vector<int>> offsetUsers;

    bool anyOrientationSet = false;
    for (const auto &si : serverConfig.screeninfo()) {
        if (si.screenorientation() != defaultOrientation)
            anyOrientationSet = true;
        offsetUsers[{si.offsetx(), si.offsety()}].push_back(si.screenid());
    }

    // Warn about screens that have orientation set but all layout fields at zero.
    // (0,0,rot0) is only plausible for the front/first panel.
    if (anyOrientationSet) {
        for (const auto &si : serverConfig.screeninfo()) {
            bool layoutIsAllZero = si.offsetx() == 0 && si.offsety() == 0 && si.screenrotation() == defaultRotation;
            bool orientationImpliesNonZeroLayout =
                si.screenorientation() != defaultOrientation && si.screenorientation() != matrixserver::ScreenInfo::front;
            if (layoutIsAllZero && orientationImpliesNonZeroLayout) {
                BOOST_LOG_TRIVIAL(warning) << "[ServerSetup] Screen " << si.screenid()
                                           << " (orientation=" << matrixserver::ScreenInfo::ScreenOrientation_Name(si.screenorientation())
                                           << ") has offsetX=0, offsetY=0, screenRotation=rot0 — layout "
                                              "fields may be missing from the config file. Delete the config "
                                              "file and restart to regenerate it with correct layout values.";
            }
        }
    }

    // Warn about duplicate (offsetX, offsetY) pairs across different screens.
    for (const auto &[offset, ids] : offsetUsers) {
        if (ids.size() > 1) {
            std::string idList;
            for (int id : ids)
                idList += std::to_string(id) + " ";
            BOOST_LOG_TRIVIAL(warning) << "[ServerSetup] Screens " << idList << "share the same offset (" << offset.first << "," << offset.second
                                       << "). This is almost certainly wrong — layout fields (offsetX, "
                                          "offsetY, screenRotation) may be missing from the config file.";
        }
    }
}

std::vector<std::shared_ptr<Screen>> createScreensFromConfig(const matrixserver::ServerConfig &serverConfig) {
    warnIfConfigSuspicious(serverConfig);
    std::vector<std::shared_ptr<Screen>> screens;
    for (const auto &screenInfo : serverConfig.screeninfo()) {
        auto screen = std::make_shared<Screen>(screenInfo.width(), screenInfo.height(), screenInfo.screenid());
        screen->setOffsetX(screenInfo.offsetx());
        screen->setOffsetY(screenInfo.offsety());
        screen->setRotation(static_cast<Rotation>(screenInfo.screenrotation()));
        screens.push_back(screen);
    }
    return screens;
}

void handleServerConfig(int argc, char **argv, matrixserver::ServerConfig &serverConfig, HardwareType hwType) {
    std::string configPath = "";
    std::string serverAddressOverride = "";

    try {
        po::options_description desc("Allowed options");
        desc.add_options()("help", "produce help message")("config", po::value<std::string>(&configPath), "path to configuration file")(
            "address", po::value<std::string>(&serverAddressOverride), "override server address");

        po::positional_options_description p;
        p.add("config", 1);

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).options(desc).positional(p).allow_unregistered().run(), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::string executableName = argc > 0 ? argv[0] : "matrixserver";

            size_t lastSlash = executableName.find_last_of("/\\");
            if (lastSlash != std::string::npos) {
                executableName = executableName.substr(lastSlash + 1);
            }

            std::cout << executableName
                      << " - Matrix Server for rendering to LED cubes and other "
                         "matrix displays.\n\n"
                      << "USAGE:\n"
                      << "  " << executableName << " [OPTIONS] [CONFIG_FILE_PATH]\n\n"
                      << "OPTIONS:\n"
                      << "  -h, --help              Show this help message.\n"
                      << "  --config <path>         Path to configuration file. "
                         "Can also be passed as a positional argument.\n"
                      << "  --address <ip>          Override the server address "
                         "specified in the config.\n\n"
                      << "EXAMPLES:\n"
                      << "  " << executableName << " --config myConfig.json\n"
                      << "  " << executableName << " default_config.json\n"
                      << "  " << executableName << " --address 192.168.1.100\n";
            exit(0);
        }
    } catch (std::exception &e) {
        BOOST_LOG_TRIVIAL(error) << "[ServerSetup] Error parsing command line arguments: " << e.what();
    }

    if (configPath.empty()) {
        std::ifstream defaultFile("matrixServerConfig.json");
        if (defaultFile.good()) {
            configPath = "matrixServerConfig.json";
        }
    }

    if (!configPath.empty()) {
        std::string fullPath = configPath;
        char resolved_path[PATH_MAX];
        if (realpath(configPath.c_str(), resolved_path) != nullptr) {
            fullPath = std::string(resolved_path);
        }

        BOOST_LOG_TRIVIAL(info) << "[ServerSetup] Trying to read config from: " << fullPath;
        std::ifstream configFileReadStream(configPath);
        std::stringstream buffer;
        buffer << configFileReadStream.rdbuf();
        if (google::protobuf::util::JsonStringToMessage(buffer.str(), &serverConfig).ok()) {
            BOOST_LOG_TRIVIAL(info) << "[ServerSetup] ServerConfig successfully loaded from: " << fullPath;
        } else {
            BOOST_LOG_TRIVIAL(warning) << "[ServerSetup] ServerConfig read failed from: " << fullPath;
        }
    } else {
        std::string configFileName = "matrixServerConfig.json";
        std::string fullPath = configFileName;
        char resolved_path[PATH_MAX];
        if (realpath(".", resolved_path) != nullptr) {
            fullPath = std::string(resolved_path) + "/" + configFileName;
        }

        std::cout << "\nNo configuration file specified or found.\n"
                  << "Would you like to create a default configuration file at " << fullPath << "? [y/N]: ";
        std::string response;
        std::getline(std::cin, response);

        if (response == "y" || response == "Y") {
            BOOST_LOG_TRIVIAL(info) << "[ServerSetup] Creating default config...";
            createDefaultCubeConfig(serverConfig, hwType);
            std::string configString;
            google::protobuf::util::JsonPrintOptions jsonOptions;
            jsonOptions.add_whitespace = true;
            if (google::protobuf::util::MessageToJsonString(serverConfig, &configString, jsonOptions).ok()) {
                std::ofstream configFileWriteStream(configFileName, std::ios_base::trunc);
                configFileWriteStream << configString;
                configFileWriteStream.close();
                BOOST_LOG_TRIVIAL(info) << "[ServerSetup] Successfully wrote default config to " << fullPath;
            }
        } else {
            BOOST_LOG_TRIVIAL(info) << "[ServerSetup] Starting with default in-memory config.";
            createDefaultCubeConfig(serverConfig, hwType);
        }
    }

    if (!serverAddressOverride.empty()) {
        if (!serverConfig.has_serverconnection()) {
            serverConfig.set_allocated_serverconnection(new matrixserver::Connection());
        }
        serverConfig.mutable_serverconnection()->set_serveraddress(serverAddressOverride);
        BOOST_LOG_TRIVIAL(info) << "[ServerSetup] Server address overridden to: " << serverAddressOverride;
    }
}

} // namespace ServerSetup
