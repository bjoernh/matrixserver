project(renderer)

set(SOURCE_FILES
        IRenderer.cpp
        IRenderer.h
        )

add_library(renderer STATIC ${SOURCE_FILES})
target_link_libraries(renderer common)
target_include_directories(renderer PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:include/matrixapplication>)

add_subdirectory(SimulatorRenderer)
if (BUILD_RASPBERRYPI)
    add_subdirectory(FPGAFTDIRenderer)
    add_subdirectory(FPGASPIRenderer)
    add_subdirectory(RGBMatrixRenderer)
endif()

set_target_properties(common PROPERTIES PUBLIC_HEADER "IRenderer.h")


install(TARGETS renderer
        EXPORT matrixapplication-targets
        ARCHIVE DESTINATION lib/static
        COMPONENT renderer
        PUBLIC_HEADER
        DESTINATION include/matrixapplication
        COMPONENT renderer
        )
