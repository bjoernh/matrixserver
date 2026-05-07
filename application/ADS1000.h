#ifndef MATRIXSERVER_ADS1000_H
#define MATRIXSERVER_ADS1000_H

#include <thread>
#include <mutex>
#include <atomic>
#include <memory>

class ADS1000 {
  public:
    ADS1000();
    ~ADS1000();
    void init();
    float getVoltage();

  private:
    void startRefreshThread();
    void internalLoop();
    int read_word_2c(int fd, char reg);
    std::unique_ptr<std::thread> thread_;
    std::mutex threadLock_;
    std::atomic<bool> running_{false};
    int fd;

    float voltage;
};

#endif // MATRIXSERVER_ADS1000_H
