#ifndef MATRIXSERVER_MPU6050_H
#define MATRIXSERVER_MPU6050_H

#include <Eigen/Dense>
#include <Eigen/StdVector>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>

class Mpu6050 {
public:
    Mpu6050();
    ~Mpu6050();
    void init();
    Eigen::Vector3i getCubeAccIntersect();
    Eigen::Vector3f getAcceleration();
    Eigen::Vector3f getGyroscope();
private:
    void startRefreshThread();
    void internalLoop();
    Eigen::Vector3f applyOrientation(float x, float y, float z) const;
    std::unique_ptr<std::thread> thread_;
    std::mutex threadLock_;
    std::atomic<bool> running_{false};
    int fd;

    float xyRotDeg_{0.0f};
    float xzRotDeg_{0.0f};
    float yzRotDeg_{0.0f};

    Eigen::Vector3f acceleration;
    Eigen::Vector3f gyroscope{0, 0, 0};
};


#endif //MATRIXSERVER_MPU6050_H
