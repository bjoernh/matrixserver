#include "../MatrixApplication.h"
#include "../Mpu6050.h"
#include <cmath>
#include <fstream>
#include <cstdio>
#include <gtest/gtest.h>
#include <google/protobuf/util/json_util.h>
#include <matrixserver.pb.h>
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
        // Ensure no leftover config file from previous tests
        std::remove("matrixServerConfig.json");
    }

    void TearDown() override { std::remove("matrixServerConfig.json"); }
};

TEST_F(Mpu6050Test, HardwareUnavailableFallsBackToMatrixApplicationStaticState) {
    auto mpu = std::make_unique<Mpu6050>();

    // Simulate MatrixServer updating the global application state via a WebSocket
    // browser payload (values in m/s^2, converted internally by 1/9.81)
    MatrixApplication::latestSimulatorImuX = 1.23f;
    MatrixApplication::latestSimulatorImuY = -4.56f;
    MatrixApplication::latestSimulatorImuZ = 9.81f;

    // With default 0° orientation, values pass through after unit conversion
    Vector3f accel = mpu->getAcceleration();

    EXPECT_NEAR(accel.x(), 1.23f * 0.1019368f, 1e-5f);
    EXPECT_NEAR(accel.y(), -4.56f * 0.1019368f, 1e-5f);
    EXPECT_NEAR(accel.z(), 9.81f * 0.1019368f, 1e-5f);
}

TEST_F(Mpu6050Test, GyroDefaultNoRotation) {
    auto mpu = std::make_unique<Mpu6050>();

    MatrixApplication::latestSimulatorGyroX = 10.0f;
    MatrixApplication::latestSimulatorGyroY = 20.0f;
    MatrixApplication::latestSimulatorGyroZ = 5.0f;

    // With default 0° orientation, gyro values pass through unchanged
    Vector3f gyro = mpu->getGyroscope();

    EXPECT_FLOAT_EQ(gyro.x(), 10.0f);
    EXPECT_FLOAT_EQ(gyro.y(), 20.0f);
    EXPECT_FLOAT_EQ(gyro.z(), 5.0f);
}

TEST_F(Mpu6050Test, XZRotationLoadedFromConfigFile) {
    // Write a temp config file with 45° XZ rotation
    {
        matrixserver::ServerConfig cfg;
        cfg.mutable_imuorientation()->set_xzrotationdeg(45.0f);
        std::string json;
        google::protobuf::util::MessageToJsonString(cfg, &json);
        std::ofstream f("matrixServerConfig.json");
        f << json;
    }

    auto mpu = std::make_unique<Mpu6050>(); // reads config in init()

    MatrixApplication::latestSimulatorGyroX = 10.0f;
    MatrixApplication::latestSimulatorGyroY = 0.0f;
    MatrixApplication::latestSimulatorGyroZ = 0.0f;

    Vector3f gyro = mpu->getGyroscope();

    // XZ rotation of 45°: input (gx=10, gy=0, gz=0)
    // XZ plane rotates x and z: rotated_x = 10*cos(45°), rotated_z = 10*sin(45°)
    const float sqrt2_2 = std::sqrt(2.0f) / 2.0f;
    EXPECT_NEAR(gyro.x(), 10.0f * sqrt2_2, 1e-4f);
    EXPECT_NEAR(gyro.z(), 10.0f * sqrt2_2, 1e-4f);
    EXPECT_FLOAT_EQ(gyro.y(), 0.0f);
}
