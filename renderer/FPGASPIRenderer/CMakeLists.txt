project(FPGASPIRenderer)

set(SOURCE_FILES
        FPGARendererRPISPI.cpp FPGARendererRPISPI.h)

add_library(FPGASPIRenderer SHARED ${SOURCE_FILES})
target_link_libraries(FPGASPIRenderer PRIVATE renderer)
target_include_directories(FPGASPIRenderer PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:include/matrixapplication>)

set_target_properties(FPGASPIRenderer PROPERTIES PUBLIC_HEADER "FPGARendererRPISPI.h")

install(TARGETS FPGASPIRenderer
        EXPORT matrixapplication-targets
        LIBRARY
        DESTINATION lib
        COMPONENT FPGASPIRenderer
        PUBLIC_HEADER
        DESTINATION include/matrixapplication
        COMPONENT FPGASPIRenderer
        )
