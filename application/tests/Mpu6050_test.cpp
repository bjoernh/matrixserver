#include "../MatrixApplication.h"
#include "../Mpu6050.h"
#include <cmath>
#include <gtest/gtest.h>
#include <memory>

// Test fixture to reset the static state before each test
class Mpu6050Test : public ::testing::Test {
protected:
  void SetUp() override {
    MatrixApplication::latestSimulatorImuX = 0.0f;
    MatrixApplication::latestSimulatorImuY = 0.0f;
    MatrixApplication::latestSimulatorImuZ = 0.0f;
    MatrixApplication::latestSimulatorGyroX = 0.0f;
    MatrixApplication::latestSimulatorGyroY = 0.0f;
    MatrixApplication::latestSimulatorGyroZ = 0.0f;
  }
};

TEST_F(Mpu6050Test,
       HardwareUnavailableFallsBackToMatrixApplicationStaticState) {
  auto mpu = std::make_unique<Mpu6050>();

  // Simulate MatrixServer updating the global application state via a WebSocket
  // browser payload
  MatrixApplication::latestSimulatorImuX = 1.23f;
  MatrixApplication::latestSimulatorImuY = -4.56f;
  MatrixApplication::latestSimulatorImuZ = 9.81f;

  // mpu->fd is -1 on non-raspberry pi environments or if wiringPi fails
  // getAcceleration() should intercept this and return the SimulatorIMU
  Vector3f accel = mpu->getAcceleration();

  EXPECT_FLOAT_EQ(accel.x(), 1.23f);
  EXPECT_FLOAT_EQ(accel.y(), -4.56f);
  EXPECT_FLOAT_EQ(accel.z(), 9.81f);
}

TEST_F(Mpu6050Test,
       GyroHardwareUnavailableFallsBackToMatrixApplicationStaticState) {
  auto mpu = std::make_unique<Mpu6050>();

  // Gyro values in deg/s — no unit conversion, same axis remapping as accel
  MatrixApplication::latestSimulatorGyroX = 10.0f;
  MatrixApplication::latestSimulatorGyroY = 20.0f;
  MatrixApplication::latestSimulatorGyroZ = 0.0f;

  // getGyroscope() applies RotateVector2d(45°) to the X-Z plane and maps
  // sensor Y to the third output component, mirroring the hardware path.
  // With gz=0: rotated_x = gx*cos(45°), rotated_z = gx*sin(45°)
  const float sqrt2_2 = std::sqrt(2.0f) / 2.0f;
  Vector3f gyro = mpu->getGyroscope();

  EXPECT_NEAR(gyro.x(), 10.0f * sqrt2_2, 1e-4f);
  EXPECT_NEAR(gyro.y(), 10.0f * sqrt2_2, 1e-4f);
  EXPECT_FLOAT_EQ(gyro.z(), 20.0f);
}
