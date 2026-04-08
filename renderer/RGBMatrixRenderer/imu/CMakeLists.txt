project(imu)

set(SOURCE_FILES Imu.cpp SerialPort.cpp)

find_package(Boost 1.58.0 REQUIRED COMPONENTS thread log)
include_directories(${Boost_INCLUDE_DIRS})

add_library(Imu SHARED ${SOURCE_FILES})
target_include_directories(Imu PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:include/matrixapplication>)
target_link_libraries(Imu PRIVATE ${Boost_LIBRARIES})

install(TARGETS Imu
        EXPORT matrixapplication-targets
        LIBRARY
        DESTINATION lib
        COMPONENT Imu
        PUBLIC_HEADER
        DESTINATION include/matrixapplication
        COMPONENT Imu
        )
#    target_include_directories(RGBMatrixRenderer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})