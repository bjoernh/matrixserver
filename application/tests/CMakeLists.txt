cmake_minimum_required(VERSION 3.10)
project(matrixserver_tests)

find_package(GTest REQUIRED)

add_executable(
    Mpu6050_test
    Mpu6050_test.cpp
    ../Mpu6050.cpp
    ../MatrixApplication.cpp
)

target_include_directories(Mpu6050_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(
    Mpu6050_test
    GTest::gtest_main
    Threads::Threads
    common
)
