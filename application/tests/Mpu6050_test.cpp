#include "../MatrixApplication.h"
#include "../Mpu6050.h"
#include <gtest/gtest.h>
#include <memory>

// Test fixture to reset the static state before each test
class Mpu6050Test : public ::testing::Test {
protected:
  void SetUp() override {
    MatrixApplication::latestSimulatorImuX = 0.0f;
    MatrixApplication::latestSimulatorImuY = 0.0f;
    MatrixApplication::latestSimulatorImuZ = 0.0f;
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
