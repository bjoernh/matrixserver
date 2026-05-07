project(server)

find_package(Boost 1.58.0 REQUIRED COMPONENTS thread log program_options)
include_directories(${Boost_INCLUDE_DIRS})

set(SOURCE_FILES
        Server.cpp
        ServerSetup.cpp
        App.cpp
        DefaultAppLauncher.cpp
        ServerBootstrap.cpp
        RendererRegistry.cpp)

add_library(server STATIC ${SOURCE_FILES})
target_link_libraries(server common renderer wsSimulatorRenderer)
target_link_libraries(server ${Boost_LIBRARIES})
target_include_directories(server PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(server PUBLIC BOOST_LOG_DYN_LINK)

# Unified server executable
add_executable(matrix_server main.cpp RendererFactory.cpp)
target_link_libraries(matrix_server server wsSimulatorRenderer)
target_compile_definitions(matrix_server PUBLIC BOOST_LOG_DYN_LINK)

foreach(BACKEND IN LISTS HARDWARE_BACKEND)
    if (BACKEND STREQUAL "FPGA_FTDI")
        target_link_libraries(matrix_server FPGAFTDIRenderer)
    elseif (BACKEND STREQUAL "FPGA_RPISPI")
        target_link_libraries(matrix_server FPGASPIRenderer)
    elseif (BACKEND STREQUAL "RGB_MATRIX")
        target_link_libraries(matrix_server RGBMatrixRenderer)
    endif()
endforeach()

install(TARGETS matrix_server DESTINATION bin)