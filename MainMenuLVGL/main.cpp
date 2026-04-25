#include <CubeApplication.h>
#include "cube/launcher.hpp"
#include "cube/matrixserver/matrixserver_platform.hpp"
#include "lvgl.h"

#include <unistd.h>

class LvglMenuApp : public CubeApplication {
public:
    // screens is the protected MatrixApplication member; we can access it here
    // as a subclass and forward the reference to MatrixServerPlatform so it
    // never has to touch the protected member directly.
    LvglMenuApp() : CubeApplication(40), plat_(screens) {
        cube::launcher_init(plat_);
    }

    ~LvglMenuApp() {
        cube::launcher_deinit();
    }

    bool loop() override {
        plat_.pump_events();
        lv_timer_handler();
        plat_.copy_framebuf_to_screens();
        return true;
    }

private:
    cube::MatrixServerPlatform plat_;
};

int main() {
    lv_init();
    LvglMenuApp app;
    app.start();
    while (1) sleep(2);
    return 0;
}
