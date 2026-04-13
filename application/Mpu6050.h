#ifndef MATRIXSERVER_MPU6050_H
#define MATRIXSERVER_MPU6050_H

#include <Eigen/Dense>
#include <Eigen/StdVector>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

class Mpu6050 {
public:
    Mpu6050();
    void init();
    Eigen::Vector3i getCubeAccIntersect();
    Eigen::Vector3f getAcceleration();
    Eigen::Vector3f getGyroscope();
private:
    void startRefreshThread();
    void internalLoop();
    Eigen::Vector3f applyOrientation(float x, float y, float z) const;
    boost::thread * thread_;
    boost::mutex threadLock_;
    int fd;

    float xyRotDeg_{0.0f};
    float xzRotDeg_{0.0f};
    float yzRotDeg_{0.0f};

    Eigen::Vector3f acceleration;
    Eigen::Vector3f gyroscope{0, 0, 0};
};


#endif //MATRIXSERVER_MPU6050_H
