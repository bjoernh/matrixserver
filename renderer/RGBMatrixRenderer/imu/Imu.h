#ifndef __IMU__
#define __IMU__
#include "SerialPort.h"
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <memory>

class Imu {
  public:
    Imu();
    ~Imu();
    bool init(std::string portname);
    float* getAcceleration();
    void startRefreshThread();
    void stopThread();
    void refresh();
    bool isInFreefall();

  protected:
    std::vector<float> parseString(std::string input);

  private:
    void internalLoop();
    float acceleration[3];
    std::unique_ptr<SerialPort> serial;
    std::unique_ptr<std::thread> thread_;
    std::mutex threadLock_;
    std::atomic<bool> threadRunning_{false};
    bool freefall;
    float threshold = 0.1;
};
#endif
