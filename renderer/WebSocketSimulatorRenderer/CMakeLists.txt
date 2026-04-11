project(WebSocketSimulatorRenderer)

find_package(Boost 1.66.0 REQUIRED COMPONENTS thread log)
include_directories(${Boost_INCLUDE_DIRS})

# boost-beast is required for WebSocket support; skip if not available
find_package(Boost 1.66.0 QUIET COMPONENTS beast)
if(Boost_BEAST_FOUND OR (EXISTS "${Boost_INCLUDE_DIR}/boost/beast"))
    set(BEAST_AVAILABLE TRUE)
else()
    # Check by trying to find the header directly
    find_file(BEAST_HEADER "boost/beast/core.hpp" HINTS ${Boost_INCLUDE_DIRS})
    if(BEAST_HEADER)
        set(BEAST_AVAILABLE TRUE)
    else()
        set(BEAST_AVAILABLE FALSE)
        message(STATUS "boost-beast not found, skipping WebSocketSimulatorRenderer")
    endif()
endif()

if(BEAST_AVAILABLE)
    set(SOURCE_FILES
        WebSocketSimulatorRenderer.cpp
    )

    add_library(wsSimulatorRenderer STATIC ${SOURCE_FILES})
    target_link_libraries(wsSimulatorRenderer renderer ${Boost_LIBRARIES} $<$<PLATFORM_ID:Linux>:rt>)
    target_include_directories(wsSimulatorRenderer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_definitions(wsSimulatorRenderer PUBLIC BOOST_LOG_DYN_LINK)
else()
    # Create a stub empty library so targets linking against it still work
    add_library(wsSimulatorRenderer INTERFACE)
    target_include_directories(wsSimulatorRenderer INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
endif()
