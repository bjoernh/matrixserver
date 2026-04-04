project(matrix_server)

add_executable(matrix_server main.cpp)
target_link_libraries(matrix_server server)
target_compile_definitions(matrix_server PUBLIC BOOST_LOG_DYN_LINK)

if (HARDWARE_BACKEND STREQUAL "FPGA_FTDI")
    target_compile_definitions(matrix_server PRIVATE BACKEND_FPGA_FTDI)
    target_link_libraries(matrix_server FPGAFTDIRenderer)
elseif (HARDWARE_BACKEND STREQUAL "FPGA_RPISPI")
    target_compile_definitions(matrix_server PRIVATE BACKEND_FPGA_RPISPI)
    target_link_libraries(matrix_server FPGASPIRenderer)
elseif (HARDWARE_BACKEND STREQUAL "RGB_MATRIX")
    target_compile_definitions(matrix_server PRIVATE BACKEND_RGB_MATRIX)
    target_link_libraries(matrix_server RGBMatrixRenderer)
endif()

install(TARGETS matrix_server DESTINATION bin)
