project(MainMenuLVGL)

# Pull in the AppLuncher submodule (defines lvgl, cube_shared, cube_platform_matrixserver).
# matrixapplication must already be defined (comes from the parent CMakeLists.txt).
add_subdirectory(AppLuncher)

add_executable(MainMenuLVGL main.cpp)
target_link_libraries(MainMenuLVGL
    PRIVATE
        matrixapplication
        cube_shared
        cube_platform_matrixserver
        lvgl
        $<$<PLATFORM_ID:Linux>:stdc++fs>
)

set_target_properties(MainMenuLVGL PROPERTIES
    INSTALL_RPATH "/usr/local/lib"
    BUILD_WITH_INSTALL_RPATH TRUE)

install(TARGETS MainMenuLVGL DESTINATION bin)
