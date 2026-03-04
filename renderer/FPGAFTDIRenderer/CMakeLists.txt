project(FPGAFTDIRenderer)

set(SOURCE_FILES
        mpsse/mpsse.c
        FPGARendererFTDI.cpp FPGARendererFTDI.h)

add_library(FPGAFTDIRenderer SHARED ${SOURCE_FILES})
find_package(PkgConfig REQUIRED)
pkg_check_modules(FTDI1 REQUIRED libftdi1)
target_include_directories(FPGAFTDIRenderer PRIVATE ${FTDI1_INCLUDE_DIRS})
target_link_libraries(FPGAFTDIRenderer PRIVATE renderer ${FTDI1_LIBRARIES})
target_include_directories(FPGAFTDIRenderer PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:include/matrixapplication>)

set_target_properties(FPGAFTDIRenderer PROPERTIES PUBLIC_HEADER "FPGARendererFTDI.h")

install(TARGETS FPGAFTDIRenderer
        EXPORT matrixapplication-targets
        LIBRARY
        DESTINATION lib
        COMPONENT FPGAFTDIRenderer
        PUBLIC_HEADER
        DESTINATION include/matrixapplication
        COMPONENT FPGAFTDIRenderer
        )
