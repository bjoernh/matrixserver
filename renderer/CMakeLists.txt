project(renderer)

set(SOURCE_FILES
        IRenderer.cpp
        IRenderer.h
        IBidirectionalRenderer.h
        )

add_library(renderer STATIC ${SOURCE_FILES})
target_link_libraries(renderer common)
target_include_directories(renderer PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:include/matrixapplication>)

add_subdirectory(WebSocketSimulatorRenderer)

if(ENABLE_FPGA_FTDI)
    add_subdirectory(FPGAFTDIRenderer)
endif()
if(ENABLE_FPGA_RPISPI)
    add_subdirectory(FPGASPIRenderer)
endif()
if(ENABLE_RGB_MATRIX)
    add_subdirectory(RGBMatrixRenderer)
endif()

set_target_properties(renderer PROPERTIES PUBLIC_HEADER "IRenderer.h;IBidirectionalRenderer.h")

install(TARGETS renderer
        EXPORT matrixapplication-targets
        ARCHIVE DESTINATION lib/static
        COMPONENT renderer
        PUBLIC_HEADER
        DESTINATION include/matrixapplication
        COMPONENT renderer
        )
