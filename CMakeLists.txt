cmake_minimum_required(VERSION 3.07)
project(matrixserver VERSION 0.3.0)


set(CMAKE_CXX_STANDARD 14)

set(Boost_FIND_QUIETLY ON) #FINALLY!!!

set(Boost_USE_MULTITHREADED TRUE) # fix warnings from newer boost versions

if (EXISTS "/boot/LICENCE.broadcom") # I expect this file to be only found on an Raspian distribution
    set(BUILD_RASPBERRYPI true)
    message("Build on Raspberry Pi is enabled")
endif ()

set(default_build_type "Release")

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
            STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
            "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif ()

set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_subdirectory(common)
add_subdirectory(renderer)
add_subdirectory(server)
add_subdirectory(application)

add_subdirectory(server_simulator)

if (BUILD_RASPBERRYPI)
    add_subdirectory(MainMenu)
    add_subdirectory(server_FPGA_RPISPI)
    add_subdirectory(server_FPGA_FTDI)
    add_subdirectory(server_RGBMatrix)
endif()
install(EXPORT matrixapplication-targets
        NAMESPACE matrixapplication::
        FILE matrixapplication-config.cmake
        DESTINATION lib/cmake/matrixapplication)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/matrixapplication-config-version.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
)
install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/matrixapplication-config-version.cmake"
        DESTINATION lib/cmake/matrixapplication
)




