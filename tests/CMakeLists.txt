project(tests)

add_executable(testAll
    tests-main.cpp
    tests-cobs.cpp
    tests-cobs-stream.cpp
    tests-color.cpp
    tests-screen.cpp
    tests-animationparams.cpp
    tests-tcp.cpp
    tests-joystick.cpp
    test-unixSocket.cpp
    ${CMAKE_SOURCE_DIR}/application/AnimationParams.cpp
)
target_link_libraries(testAll common)
target_include_directories(testAll PRIVATE ${CMAKE_SOURCE_DIR}/application)
