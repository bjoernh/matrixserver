#include "Imu.h"
#include <cmath>

Imu::Imu() {}

Imu::~Imu() {
    stopThread();
    // serial unique_ptr cleans up automatically
}

bool Imu::init(std::string portname) {
    serial = std::make_unique<SerialPort>(portname.c_str(), 115200);
    startRefreshThread();
    return true;
}

std::vector<float> Imu::parseString(std::string input) {
    std::vector<float> result;
    std::istringstream sstream(input);
    for (std::string part; std::getline(sstream, part, ',');) {
        result.push_back(std::stof(part));
    }
    return result;
}

float* Imu::getAcceleration() { return acceleration; }

bool Imu::isInFreefall() { return freefall; }

void Imu::refresh() {
    std::vector<float> result = parseString(serial->readLine());
    if (result.size() == 3) {
        acceleration[0] = result[0];
        acceleration[1] = result[1];
        acceleration[2] = result[2];
        if (std::abs(acceleration[0]) < threshold && std::abs(acceleration[1]) < threshold && std::abs(acceleration[2]) < threshold)
            freefall = true;
        else
            freefall = false;
    }
}

void Imu::startRefreshThread() {
    threadRunning_.store(true);
    thread_ = std::make_unique<std::thread>(&Imu::internalLoop, this);
}

void Imu::stopThread() {
    threadRunning_.store(false);
    if (thread_ && thread_->joinable()) {
        thread_->join();
    }
    thread_.reset();
}

void Imu::internalLoop() {
    while (threadRunning_.load()) {
        refresh();
        usleep(1000);
    }
}
