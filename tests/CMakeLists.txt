project(tests)

add_executable(testAll EXCLUDE_FROM_ALL
    tests-main.cpp
    tests-cobs.cpp
    tests-cobs-stream.cpp
    tests-color.cpp
    tests-screen.cpp
    tests-animationparams.cpp
    tests-tcp.cpp
    tests-joystick.cpp
    test-unixSocket.cpp
    tests-message-dispatcher.cpp
    tests-connection-factory.cpp
    tests-input-state.cpp
    ${CMAKE_SOURCE_DIR}/application/AnimationParams.cpp
)
target_link_libraries(testAll matrixapplication)
target_include_directories(testAll PRIVATE ${CMAKE_SOURCE_DIR}/application)
