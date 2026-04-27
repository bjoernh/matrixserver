#include "ADS1000.h"

#include <stdio.h>
#include <stdint.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <iostream>
#include <unistd.h>
#include <byteswap.h>
#include <sys/stat.h>
#include <boost/log/trivial.hpp>

// values
// 944 = 10.05V
// 1412 = 15.02V
// 1613 = 17.15V
// 632 = 6.71V
// linear interpoliert: voltage = 0,0106 * value - 0,0055

#define ADS1000_I2C_ADDRESS 0x48 // I2C

int ADS1000::read_word_2c(int fd, char reg) {
    uint16_t value = wiringPiI2CReadReg16(fd, reg);
    value = __bswap_16(value);
    if (value >= 0x8000)
        return -((65535 - value) + 1);
    else
        return value;
}

ADS1000::ADS1000() { init(); }

ADS1000::~ADS1000() {
    running_.store(false);
    if (thread_ && thread_->joinable()) {
        thread_->join();
    }
}

void ADS1000::init() {
    struct stat buffer;
    if (stat("/dev/i2c-1", &buffer) != 0) {
        BOOST_LOG_TRIVIAL(warning) << "[ADS1000] I2C device /dev/i2c-1 not found, battery monitoring disabled";
        fd = -1;
        return;
    }

    fd = wiringPiI2CSetup(ADS1000_I2C_ADDRESS);
    if (fd == -1)
        return;

    int readval = read_word_2c(fd, 0x00); // do one dummy read

    startRefreshThread();
}

void ADS1000::startRefreshThread() {
    running_.store(true);
    thread_ = std::make_unique<std::thread>(&ADS1000::internalLoop, this);
}

void ADS1000::internalLoop() {
    int loopcount = 0;
    while (running_.load()) {
        int readval = read_word_2c(fd, 0x00);
        voltage = (0.0106f * readval) - 0.0055f;
        usleep(100000); // 10fps
    }
}

float ADS1000::getVoltage() { return voltage; }