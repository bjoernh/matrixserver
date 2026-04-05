project(matrix_server_simulator)

find_package(Boost 1.58.0 REQUIRED COMPONENTS thread log)
include_directories(${Boost_INCLUDE_DIRS})

set (CMAKE_SHARED_LINKER_FLAGS "-undefined dynamic_lookup")

add_executable(matrix_server_simulator main.cpp)
target_link_libraries(matrix_server_simulator server wsSimulatorRenderer $<$<PLATFORM_ID:Linux>:rt>)

target_compile_definitions(matrix_server_simulator PUBLIC BOOST_LOG_DYN_LINK)

install(TARGETS matrix_server_simulator DESTINATION bin)
