project(MainMenu)

set(MAINSRC
        main.cpp
        mainmenu.cpp)

set(MAINLIBS
        matrixapplication
        $<$<PLATFORM_ID:Linux>:stdc++fs>
        )

add_executable(MainMenu ${MAINSRC})
target_link_libraries(MainMenu ${MAINLIBS})

set_target_properties(MainMenu PROPERTIES
        INSTALL_RPATH "/usr/local/lib"
        BUILD_WITH_INSTALL_RPATH TRUE)

install(TARGETS MainMenu DESTINATION bin)