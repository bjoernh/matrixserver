project(WebSocketSimulatorRenderer)

find_package(Boost 1.66.0 REQUIRED COMPONENTS log)
include_directories(${Boost_INCLUDE_DIRS})

set(SOURCE_FILES
    WebSocketSimulatorRenderer.cpp
)

add_library(wsSimulatorRenderer STATIC ${SOURCE_FILES})
target_link_libraries(wsSimulatorRenderer renderer ${Boost_LIBRARIES} $<$<PLATFORM_ID:Linux>:rt>)
target_include_directories(wsSimulatorRenderer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(wsSimulatorRenderer PUBLIC BOOST_LOG_DYN_LINK)
