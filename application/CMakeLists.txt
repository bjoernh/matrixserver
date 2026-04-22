project(application)

find_package(Boost 1.58.0 REQUIRED COMPONENTS thread log)
include_directories(${Boost_INCLUDE_DIRS})

find_package(Eigen3 REQUIRED NO_MODULE)

IF(APPLE)
	# Fix linking on 10.14+. See https://stackoverflow.com/questions/54068035
	# for sucessfull linking against lmlib2
	LINK_DIRECTORIES(/usr/local/lib /opt/homebrew/lib)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/opt/X11/include -I/opt/homebrew/include -I/opt/homebrew/include/eigen3")
ENDIF()


set(SOURCE_FILES
        CubeApplication.cpp
        Mpu6050.cpp
        AnimationParams.cpp)

if (BUILD_RASPBERRYPI)
	set(SOURCE_FILES
			${SOURCE_FILES}
        	ADS1000.cpp ADS1000.h)
endif ()


add_library(matrixapplication SHARED ${SOURCE_FILES} MatrixApplication.cpp)

target_link_libraries(matrixapplication common renderer Eigen3::Eigen)

target_link_libraries(matrixapplication
        ${Boost_LIBRARIES}
        $<$<PLATFORM_ID:Linux>:rt>
         )
if (BUILD_RASPBERRYPI)
    target_compile_definitions(matrixapplication PUBLIC BUILD_RASPBERRYPI)
    target_link_libraries(matrixapplication wiringPi)
endif ()

if (HARDWARE_BACKEND STREQUAL "FPGA_RPISPI")
    target_link_libraries(matrixapplication FPGASPIRenderer)
elseif (HARDWARE_BACKEND STREQUAL "RGB_MATRIX")
    target_link_libraries(matrixapplication RGBMatrixRenderer)
elseif (HARDWARE_BACKEND STREQUAL "FPGA_FTDI")
    target_link_libraries(matrixapplication FPGAFTDIRenderer)
endif ()

target_include_directories(matrixapplication PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:include/matrixapplication>)


target_compile_definitions(matrixapplication PUBLIC BOOST_LOG_DYN_LINK)
set_target_properties(matrixapplication PROPERTIES PUBLIC_HEADER "CubeApplication.h;Font6px.h;Mpu6050.h;ADS1000.h;AnimationParams.h;MatrixApplication.h")

install(TARGETS matrixapplication
        EXPORT matrixapplication-targets
        LIBRARY
        DESTINATION lib
        COMPONENT matrixapplication
        PUBLIC_HEADER
        DESTINATION include/matrixapplication
        COMPONENT matrixapplication
        )

