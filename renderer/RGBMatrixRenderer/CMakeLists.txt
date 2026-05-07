project(RGBMatrixRenderer)

if (NOT BUILD_RASPBERRYPI)
    message(FATAL_ERROR
        "RGB_MATRIX backend requires Raspberry Pi (BUILD_RASPBERRYPI is set "
        "automatically when /boot/LICENCE.broadcom exists). To build on a "
        "non-Pi host (e.g. CI or cross-build), pass -DBUILD_RASPBERRYPI=ON, "
        "or remove RGB_MATRIX from HARDWARE_BACKEND.")
endif ()

set(RGB_LIB_DISTRIBUTION rpi-rgb-led-matrix)
set(RGB_INCDIR ${RGB_LIB_DISTRIBUTION}/include)
set(RGB_LIBDIR ${RGB_LIB_DISTRIBUTION}/lib)
set(RGB_LIBRARY_NAME rgbmatrix)

include_directories(${RGB_INCDIR})
link_directories(${RGB_LIBDIR})
set(RGB_LIBRARIES ${RGB_LIBRARY_NAME} rt m)

set(SOURCE_FILES
        RGBMatrixRenderer.cpp
        )

add_library(RGBMatrixRenderer SHARED ${SOURCE_FILES})
target_link_libraries(RGBMatrixRenderer PRIVATE renderer Imu ${RGB_LIBRARIES})
target_include_directories(RGBMatrixRenderer PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:include/matrixapplication>)
target_compile_definitions(RGBMatrixRenderer INTERFACE HAVE_RGB_MATRIX)

add_custom_command(TARGET RGBMatrixRenderer PRE_LINK COMMAND "make" WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/${RGB_LIB_DISTRIBUTION}/lib)

install(TARGETS RGBMatrixRenderer
        EXPORT matrixapplication-targets
        LIBRARY
        DESTINATION lib
        COMPONENT RGBMatrixRenderer
        PUBLIC_HEADER
        DESTINATION include/matrixapplication
        COMPONENT RGBMatrixRenderer
        )

add_subdirectory(imu)
