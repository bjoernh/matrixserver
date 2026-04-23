#include "Joystick.h"
#include "matrixserver.pb.h"
#include <boost/log/trivial.hpp>
#include <boost/thread/thread.hpp>
#include <boost/chrono.hpp>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <sstream>
#include "unistd.h"

std::map<int, Joystick::SimulatorState> Joystick::simulatorStates_;
std::map<int, std::chrono::steady_clock::time_point> Joystick::simulatorLastUpdate_;
boost::mutex Joystick::simulatorMutex_;

Joystick::Joystick()
{
    init("/dev/input/js0", false);
}

Joystick::Joystick(int joystickNumber)
{
    std::stringstream sstm;
    sstm << "/dev/input/js" << joystickNumber;
    init(sstm.str(), false);
}

Joystick::Joystick(std::string devicePath)
{
    init(devicePath, false);
}

Joystick::Joystick(std::string devicePath, bool blocking)
{
    init(devicePath, blocking);
}

void Joystick::resetVariables(){
    for(int i = 0; i < MAXBUTTONAXISCOUNT; i++){
        buttonPress_[i] = 0;
        button_[i] = 0;
        axis_[i] = 0;
    }
}

void Joystick::init(std::string devicePath, bool blocking){
    _fd = -1;
    thread_ = nullptr;
    resetVariables();
    devicePath_ = devicePath;
    blocking_ = blocking;
    useSimulatorFallback_ = false;
    joystickNumber_ = 0;

    if (devicePath.find("/dev/input/js") != std::string::npos) {
        try { joystickNumber_ = std::stoi(devicePath.substr(13)); } catch(...) {}
    }

    struct stat buffer;
    if (stat(devicePath_.c_str(), &buffer) != 0) {
        useSimulatorFallback_ = true;
        BOOST_LOG_TRIVIAL(debug) << "Joystick " << joystickNumber_ << " not found (" << devicePath << "). Falling back to simulator mode.";
        startRefreshThread();
    } else {
        openPath();
        startRefreshThread();
    }
}

void Joystick::openPath()
{
    struct stat buffer;
    if (stat (devicePath_.c_str(), &buffer) == 0){
        sleep(1);
        _fd = open(devicePath_.c_str(), blocking_ ? O_RDONLY : O_RDONLY | O_NONBLOCK);
    }
}

void Joystick::recheckFilePath()
{
    struct stat buffer;
    bool fileExists = (stat(devicePath_.c_str(), &buffer) == 0);

    if (useSimulatorFallback_) {
        // Late-pairing: if a physical device has appeared while we were in fallback, transition out.
        if (fileExists) {
            BOOST_LOG_TRIVIAL(info) << "Joystick " << joystickNumber_
                                    << " device appeared, switching out of simulator fallback.";
            resetVariables();
            openPath();
            useSimulatorFallback_ = false;
        }
        return;
    }

    if (!fileExists) {
        _fd = 0;
    }
    if (_fd == 0) {
        resetVariables();
        openPath();
    }
}

bool Joystick::sample(Joystick::Event * event)
{
    if(_fd > 0){
        int bytes = read(_fd, event, sizeof(*event));
        if (bytes == -1)
            return false;
        return bytes == sizeof(*event);
    }
    return false;
}

bool Joystick::isFound()
{
    // For simulator-fallback slots, only report active once recent input has been received.
    // This prevents idle slots from forcing games into unwanted multiplayer mode.
    if (useSimulatorFallback_) {
        boost::mutex::scoped_lock lock(simulatorMutex_);
        auto it = simulatorLastUpdate_.find(joystickNumber_);
        if (it == simulatorLastUpdate_.end()) return false;
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - it->second).count();
        return elapsed < SIMULATOR_ACTIVITY_TIMEOUT_MS;
    }
    struct stat buffer;
    return (stat (devicePath_.c_str(), &buffer) == 0);
}

void Joystick::refresh()
{
    Joystick::Event event;
    while (sample(&event))
    {
        if(event.number < MAXBUTTONAXISCOUNT){
            if (event.isButton()){
                if(!button_[event.number] && (bool)event.value)
                    buttonPress_[event.number] = true;
                button_[event.number] = (bool)event.value;
            }else if (event.isAxis()){
                auto tempVal = (float)event.value / INT16_MAX;
                if(axis_[event.number] == 0 && tempVal != 0)
                    axisPress_[event.number] = tempVal;
                axis_[event.number] = tempVal;
            }
        }
    }
}

void Joystick::startRefreshThread()
{
    thread_ = new boost::thread(&Joystick::internalLoop, this);
}

void Joystick::stopThread()
{
    if(thread_ != NULL)
    {
        thread_->interrupt();
        thread_->join();
        thread_ = NULL;
    }
}

void Joystick::internalLoop()
{
    int loopcount = 0;
    while(1){
        if(loopcount % 10 == 0)
            recheckFilePath();
        refresh();
        loopcount++;
        // Use boost sleep so thread interruption in stopThread() works correctly.
        boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
    }
}


bool Joystick::getButton(unsigned int num)
{
    if (useSimulatorFallback_) {
        boost::mutex::scoped_lock lock(simulatorMutex_);
        if (num < MAXBUTTONAXISCOUNT) return simulatorStates_[joystickNumber_].button_[num];
        return false;
    }
    if(num < MAXBUTTONAXISCOUNT)
        return button_[num];
    return false;
}

bool Joystick::getButtonPress(unsigned int num)
{
    if (useSimulatorFallback_) {
        boost::mutex::scoped_lock lock(simulatorMutex_);
        if(num < MAXBUTTONAXISCOUNT){
            if(simulatorStates_[joystickNumber_].buttonPress_[num]){
                simulatorStates_[joystickNumber_].buttonPress_[num] = false;
                return true;
            }
        }
        return false;
    }
    if(num < MAXBUTTONAXISCOUNT){
        if(buttonPress_[num]){
            buttonPress_[num] = false;
            return true;
        }
    }
    return false;
}

float Joystick::getAxis(unsigned int num)
{
    if (useSimulatorFallback_) {
        boost::mutex::scoped_lock lock(simulatorMutex_);
        if (num < MAXBUTTONAXISCOUNT) return simulatorStates_[joystickNumber_].axis_[num];
        return 0.0f;
    }
    if(num < MAXBUTTONAXISCOUNT)
        return axis_[num];
    return 0;
}

float Joystick::getAxisPress(unsigned int num) {
    if (useSimulatorFallback_) {
        boost::mutex::scoped_lock lock(simulatorMutex_);
        if(num < MAXBUTTONAXISCOUNT){
            if(simulatorStates_[joystickNumber_].axisPress_[num] != 0){
                auto returnValue = simulatorStates_[joystickNumber_].axisPress_[num];
                simulatorStates_[joystickNumber_].axisPress_[num] = 0.0f;
                return returnValue;
            }
        }
        return 0.0f;
    }
    if(num < MAXBUTTONAXISCOUNT){
        if(axisPress_[num] != 0){
            auto returnValue = axisPress_[num];
            axisPress_[num] = 0.0f;
            return returnValue;
        }
    }
    return 0.0f;
}


void Joystick::clearAllButtonPresses(){
    if (useSimulatorFallback_) {
        boost::mutex::scoped_lock lock(simulatorMutex_);
        for(int i = 0; i < MAXBUTTONAXISCOUNT; i++){
            simulatorStates_[joystickNumber_].buttonPress_[i] = false;
            simulatorStates_[joystickNumber_].axisPress_[i] = 0.0f;
        }
        return;
    }
    for(int i = 0; i < MAXBUTTONAXISCOUNT; i++){
        buttonPress_[i] = false;
    }
    for(int i = 0; i < MAXBUTTONAXISCOUNT; i++){
        axisPress_[i] = 0.0f;
    }
}

void Joystick::updateSimulatorState(const matrixserver::JoystickData& data) {
    // Record activity timestamp for lazy activation: isFound() uses this to determine
    // whether a simulator-fallback slot has received input recently enough to be considered active.
    boost::mutex::scoped_lock lock(simulatorMutex_);
    int id = data.joystickid();
    simulatorLastUpdate_[id] = std::chrono::steady_clock::now();
    auto& state = simulatorStates_[id];
    
    auto updateBtn = [](bool& btn, bool& press, bool newVal) {
        if (!btn && newVal) press = true;
        btn = newVal;
    };
    auto updateAxis = [](float& axis, float& press, float newVal) {
        if (axis == 0.0f && newVal != 0.0f) press = newVal;
        axis = newVal;
    };

    updateBtn(state.button_[0], state.buttonPress_[0], data.buttona());
    updateBtn(state.button_[1], state.buttonPress_[1], data.buttonb());
    updateBtn(state.button_[2], state.buttonPress_[2], data.buttonx());
    updateBtn(state.button_[3], state.buttonPress_[3], data.buttony());
    updateBtn(state.button_[4], state.buttonPress_[4], data.buttonl());
    updateBtn(state.button_[5], state.buttonPress_[5], data.buttonr());
    updateBtn(state.button_[6], state.buttonPress_[6], data.buttonselect());
    updateBtn(state.button_[7], state.buttonPress_[7], data.buttonstart());
    updateBtn(state.button_[8], state.buttonPress_[8], data.leftstickbutton());
    updateBtn(state.button_[9], state.buttonPress_[9], data.rightstickbutton());
    
    float dpadx = 0.0f;
    if (data.buttondpadleft()) dpadx = -1.0f;
    else if (data.buttondpadright()) dpadx = 1.0f;
    
    float dpady = 0.0f;
    if (data.buttondpadup()) dpady = -1.0f;
    else if (data.buttondpaddown()) dpady = 1.0f;

    updateAxis(state.axis_[0], state.axisPress_[0], data.axisx());
    updateAxis(state.axis_[1], state.axisPress_[1], data.axisy());
    updateAxis(state.axis_[2], state.axisPress_[2], data.lefttrigger());
    updateAxis(state.axis_[3], state.axisPress_[3], data.rightaxisx());
    updateAxis(state.axis_[4], state.axisPress_[4], data.rightaxisy());
    updateAxis(state.axis_[5], state.axisPress_[5], data.righttrigger());
    updateAxis(state.axis_[6], state.axisPress_[6], dpadx);
    updateAxis(state.axis_[7], state.axisPress_[7], dpady);
}

Joystick::~Joystick()
{
    stopThread();
    close(_fd);
}


std::ostream& operator<<(std::ostream& os, const Joystick::Event& e)
{
    os << "type=" << static_cast<int>(e.type)
       << " number=" << static_cast<int>(e.number)
       << " value=" << static_cast<int>(e.value);
    return os;
}

JoystickManager::JoystickManager(unsigned int maxNum) {
    for(int i = 0; i<maxNum; i++)
        joysticks.push_back(new Joystick(i));
}

std::vector<Joystick *> &JoystickManager::getJoysticks() {
    return joysticks;
}

bool JoystickManager::getButtonPress(unsigned int num) {
    bool returnValue = false;
    for(auto joystick : joysticks){
        returnValue = (returnValue || joystick->getButtonPress(num));
    }
    return returnValue;
}

float JoystickManager::getAxis(unsigned int num) {
    float returnValue = 0;
    for(auto joystick : joysticks){
        returnValue += joystick->getAxis(num);
    }
    return returnValue;
}

float JoystickManager::getAxisPress(unsigned int num) {
    float returnValue = 0;
    for(auto joystick : joysticks){
        returnValue += joystick->getAxisPress(num);
    }
    return returnValue;
}

void JoystickManager::clearAllButtonPresses() {
    for(auto joystick : joysticks){
        joystick->clearAllButtonPresses();
    }
}
