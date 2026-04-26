#ifndef MATRIXSERVER_JOYSTICK_H
#define MATRIXSERVER_JOYSTICK_H

#include <string>
#include <iostream>
#include <map>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <memory>
#include <array>
#include <vector>

inline constexpr int MAXBUTTONAXISCOUNT = 16;

namespace matrixserver {
    class JoystickData;
}

class Joystick {
public:
    class Event;

    struct SimulatorState {
        std::array<bool, MAXBUTTONAXISCOUNT> button_;
        std::array<bool, MAXBUTTONAXISCOUNT> buttonPress_;
        std::array<float, MAXBUTTONAXISCOUNT> axis_;
        std::array<float, MAXBUTTONAXISCOUNT> axisPress_;
        
        SimulatorState() {
            button_.fill(false);
            buttonPress_.fill(false);
            axis_.fill(0.0f);
            axisPress_.fill(0.0f);
        }
    };

    /// Timeout in milliseconds after which a simulator-fallback slot is considered inactive if no
    /// simulator input has been received. Used by isFound() to prevent idle slots from appearing active.
    static constexpr int SIMULATOR_ACTIVITY_TIMEOUT_MS = 3000;

    static void updateSimulatorState(const matrixserver::JoystickData& data);

    ~Joystick();

    Joystick();

    Joystick(int joystickNumber);

    Joystick(std::string devicePath);

    Joystick(Joystick const &) = delete;


    Joystick(std::string devicePath, bool blocking);

    void init(std::string devicePath, bool blocking);

    /// Returns true if the joystick is available. For simulator-fallback slots, returns true only
    /// after recent simulator input has been received (within SIMULATOR_ACTIVITY_TIMEOUT_MS).
    bool isFound();

    bool sample(Joystick::Event *event);

    void startRefreshThread();

    void stopThread();

    void refresh();

    bool getButton(unsigned int num);

    bool getButtonPress(unsigned int num);

    float getAxis(unsigned int num);

    float getAxisPress(unsigned int num);

    void clearAllButtonPresses();

private:
    void internalLoop();

    void openPath();

    void recheckFilePath();

    void resetVariables();

    int _fd;
    std::unique_ptr<std::thread> thread_;
    std::mutex threadLock_;
    std::atomic<bool> threadRunning_{false};
    std::string devicePath_;
    bool blocking_;
    std::array<bool, MAXBUTTONAXISCOUNT> button_;
    std::array<bool, MAXBUTTONAXISCOUNT> buttonPress_;
    std::array<float, MAXBUTTONAXISCOUNT> axis_;
    std::array<float, MAXBUTTONAXISCOUNT> axisPress_;

    int joystickNumber_;
    bool useSimulatorFallback_;

    static std::map<int, SimulatorState> simulatorStates_;
    static std::map<int, std::chrono::steady_clock::time_point> simulatorLastUpdate_;
    static std::mutex simulatorMutex_;
};

class Joystick::Event {
public:
    unsigned int time;
    short value;
    unsigned char type;
    unsigned char number;

    bool isButton() {
        return (type & 0x01) != 0;
    }

    bool isAxis() {
        return (type & 0x02) != 0;
    }

    bool isInitialState() {
        return (type & 0x80) != 0;
    }

    friend std::ostream &operator<<(std::ostream &os, const Joystick::Event &e);
};

std::ostream &operator<<(std::ostream &os, const Joystick::Event &e);

class JoystickManager {
public:
    JoystickManager(unsigned int maxNum = 8);
    ~JoystickManager() = default; // unique_ptr handles cleanup

    // Returns a snapshot vector of raw pointers for callers that iterate.
    // Ownership remains with JoystickManager; pointers are valid as long as the
    // JoystickManager is alive and no joysticks are added/removed.
    std::vector<Joystick *> getJoysticks() const;

    bool getButtonPress(unsigned int num);

    float getAxis(unsigned int num);

    float getAxisPress(unsigned int num);

    void clearAllButtonPresses();

private:
    std::vector<std::unique_ptr<Joystick>> joysticks;

};


#endif //MATRIXSERVER_JOYSTICK_H
