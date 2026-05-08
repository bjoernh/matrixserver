cmake_minimum_required(VERSION 3.07)
project(matrixserver VERSION 0.5.0)

if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

set(CMAKE_CXX_STANDARD 20)

set(Boost_USE_MULTITHREADED TRUE)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

if (EXISTS "/boot/LICENCE.broadcom")
  set(BUILD_RASPBERRYPI true)
  message("Build on Raspberry Pi is enabled")
endif ()

set(default_build_type "Release")

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
            STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
            "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif ()

set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# Hardware backend selection (may be a semicolon-separated list)
set(HARDWARE_BACKEND "" CACHE STRING "Hardware renderer backend(s): FPGA_FTDI, FPGA_RPISPI, RGB_MATRIX (may be a semicolon-separated list)")
set(VALID_BACKENDS "FPGA_FTDI" "FPGA_RPISPI" "RGB_MATRIX")

foreach(BACKEND IN LISTS HARDWARE_BACKEND)
  if (NOT BACKEND IN_LIST VALID_BACKENDS)
    message(FATAL_ERROR "Invalid HARDWARE_BACKEND entry '${BACKEND}'. "
              "Must be one of: FPGA_FTDI, FPGA_RPISPI, RGB_MATRIX")
  endif ()
endforeach()

# Core libraries
add_subdirectory(common)
add_subdirectory(renderer)
add_subdirectory(server)
add_subdirectory(application)

add_subdirectory(MainMenu)

# Tests
add_subdirectory(tests)

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
