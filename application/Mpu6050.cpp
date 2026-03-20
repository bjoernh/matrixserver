#include "Mpu6050.h"

#include "MatrixApplication.h"
#ifdef __APPLE__
#include <libkern/OSByteOrder.h>
#define __bswap_16 OSSwapInt16
#else
#include <byteswap.h>
#endif
#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#ifdef BUILD_RASPBERRYPI
#include <wiringPi.h>
#include <wiringPiI2C.h>
#else
// Stub wiringPi for non-RPi builds
inline int wiringPiI2CSetup(int addr) { return -1; }
inline int wiringPiI2CReadReg8(int fd, int reg) { return -1; }
inline int wiringPiI2CReadReg16(int fd, int reg) { return -1; }
inline int wiringPiI2CWriteReg16(int fd, int reg, int val) { return -1; }
#endif

#include <cmath>
#define PI 3.14159265

using namespace Eigen;

#define MPU6050_GYRO_XOUT_H 0x43 // R
#define MPU6050_GYRO_YOUT_H 0x45 // R
#define MPU6050_GYRO_ZOUT_H 0x47 // R

#define MPU6050_ACCEL_XOUT_H 0x3B // R
#define MPU6050_ACCEL_YOUT_H 0x3D // R
#define MPU6050_ACCEL_ZOUT_H 0x3F // R

#define MPU6050_PWR_MGMT_1 0x6B  // R/W
#define MPU6050_I2C_ADDRESS 0x68 // I2C

Vector2f RotateVector2d(Vector2f input, double degrees) {
  Vector2f result;
  double radians = degrees * PI / 180;
  result[0] = input[0] * cos(radians) - input[1] * sin(radians);
  result[1] = input[0] * sin(radians) + input[1] * cos(radians);
  return result;
}

int read_word_2c(int fd, char reg) {
  uint16_t value = wiringPiI2CReadReg16(fd, reg);
  value = __bswap_16(value);
  if (value >= 0x8000)
    return -((65535 - value) + 1);
  else
    return value;
}

Mpu6050::Mpu6050() { init(); }

Vector3i Mpu6050::getCubeAccIntersect() {
  int maxC;
  auto scaler = acceleration.cwiseAbs().maxCoeff(&maxC);
  auto scaledAcc = acceleration * (1.0f / scaler);

  Vector3i accCubePos =
      (scaledAcc * 33).template cast<int>() + Vector3i(33, 33, 33);

  return accCubePos;
}

void Mpu6050::init() {
  fd = wiringPiI2CSetup(MPU6050_I2C_ADDRESS);
  if (fd == -1) {
    std::cout
        << "[Mpu6050] I2C Hardware not found. Injected IMU data will be used."
        << std::endl;
    return;
  }

  wiringPiI2CReadReg8(fd, MPU6050_PWR_MGMT_1);
  wiringPiI2CWriteReg16(fd, MPU6050_PWR_MGMT_1, 0);

  startRefreshThread();
}

void Mpu6050::startRefreshThread() {
  thread_ = new boost::thread(&Mpu6050::internalLoop, this);
}

void Mpu6050::internalLoop() {
  if (fd == -1)
    return;
  int loopcount = 0;
  while (1) {
    float gx, gy, gz, ax, ay, az;

    gx = read_word_2c(fd, MPU6050_GYRO_XOUT_H) / 131.0f;
    gy = read_word_2c(fd, MPU6050_GYRO_YOUT_H) / 131.0f;
    gz = read_word_2c(fd, MPU6050_GYRO_ZOUT_H) / 131.0f;

    ax = read_word_2c(fd, MPU6050_ACCEL_XOUT_H) / 16384.0f;
    ay = read_word_2c(fd, MPU6050_ACCEL_YOUT_H) / 16384.0f;
    az = read_word_2c(fd, MPU6050_ACCEL_ZOUT_H) / 16384.0f;

    Vector2f temp;
    temp[0] = ax;
    temp[1] = az;
    auto rotated = RotateVector2d(temp, 45.0f);

    acceleration[0] = rotated[0];
    acceleration[1] = rotated[1];
    acceleration[2] = ay;

    usleep(10000);
  }
}

Vector3f Mpu6050::getAcceleration() {
  if (fd == -1) {
    // Fallback to simulator / smartphone injected IMU data if I2C hardware is
    // missing
    std::lock_guard<std::mutex> lock(MatrixApplication::simulatorImuMutex);
    // Smartphone JS values are in m/s^2 (~9.8 for gravity).
    // The C++ Mpu6050 code expects normalized 1.0g = ~1.0 scale based on the
    // 16384.0f divisor.
    float ax = MatrixApplication::latestSimulatorImuX * 0.1019368f; // 1 / 9.81
    float ay = MatrixApplication::latestSimulatorImuY * 0.1019368f;
    float az = MatrixApplication::latestSimulatorImuZ * 0.1019368f;

    Vector2f temp;
    temp[0] = ax;
    temp[1] = az;
    auto rotated = RotateVector2d(temp, 45.0f);

    return Vector3f(rotated[0], rotated[1], ay);
  }
  return acceleration;
}