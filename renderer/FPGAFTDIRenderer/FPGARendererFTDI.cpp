#include "FPGARendererFTDI.h"

#include <cstring>
#include <cstdlib>

extern "C" {
    #include "mpsse/mpsse.h"
}

static void set_cs(int cs_b)
{
    uint8_t gpio = 0;
    uint8_t direction = 0x2b;

    /*
     * XXX
     * The chip select here is the dedicated SPI chip select.
     * I am not sure how it is being toggled by hand yet.
     * Not sure this will work.
     */
    if (cs_b) {
        gpio |= 0x28;
    }

    mpsse_set_gpio(gpio, direction);
}

FPGARendererFTDI::FPGARendererFTDI() {

}

FPGARendererFTDI::FPGARendererFTDI(std::vector<std::shared_ptr<Screen>> initScreens) {
    init(initScreens);
}

void FPGARendererFTDI::init(std::vector<std::shared_ptr<Screen>> initScreens) {
    screens = initScreens;

    std::cout << "Init SPI Driver" << std::endl;
    mpsse_init(0, nullptr, false);

    // Pre-allocate render buffers so render() never calls malloc per frame.
    if (!screens.empty()) {
        const int screenWidth  = screens[0]->getWidth();
        const int screenHeight = screens[0]->getHeight();
        const int bitDepthInBytes = 2;
        const int screenCount  = static_cast<int>(screens.size());
        const int llen = screenWidth * bitDepthInBytes * screenCount;
        const int flen = screenHeight * llen;
        frameBuf_.resize(static_cast<size_t>(flen), 0);
        cmdBuf_.resize(static_cast<size_t>(llen + 128), 0);
    }
}

void FPGARendererFTDI::setScreenData(int screenId, Color *screenData) {
    if(!renderMutex.try_lock())
        return;
    if (screenId < screens.size()) {
        screens.at(screenId)->setScreenData(screenData);
    }
    renderMutex.unlock();
}

void FPGARendererFTDI::render() {
    if(!renderMutex.try_lock())
        return;
//    auto usStart = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());

    const int screenWidth = screens[0]->getWidth();
    const int screenHeight = screens[0]->getHeight();
//    const int bitDepthInBytes = sizeof(Color);
    const int bitDepthInBytes = 2;
    const int screenCount = screens.size();

    int llen = screenWidth * bitDepthInBytes * screenCount;
    int flen = screenHeight * llen;

    // Grow buffers lazily if screen geometry changed; normal case is no-op.
    if (static_cast<int>(frameBuf_.size()) < flen)
        frameBuf_.resize(static_cast<size_t>(flen), 0);
    if (static_cast<int>(cmdBuf_.size()) < llen + 128)
        cmdBuf_.resize(static_cast<size_t>(llen + 128), 0);

    uint8_t *buf = frameBuf_.data();
    uint8_t *cmd_buf = cmdBuf_.data();

    /* Doing VSync first */
#if 1
    do {
        cmd_buf[0] = 0x00;
        cmd_buf[1] = 0x00;
        set_cs(0);
        mpsse_xfer_spi(cmd_buf, 2);
        set_cs(1);
//        printf("%d\n", cmd_buf[0] | cmd_buf[1]);
    } while (((cmd_buf[0] | cmd_buf[1]) & 0x02) != 0x02);
#endif

//    auto usTotal2 = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()) - usStart;
//    std::cout << "vsync:  " << usTotal2.count() << " us" << std::endl;

    /* Upload all the lines */
    for (int y=0; y<screenHeight; y++)
    {
        int i=0;

        /* Set CS low */
        cmd_buf[i++] = 0x80; /* MC_SETB_LOW */
        cmd_buf[i++] = 0x00; /* gpio */
        cmd_buf[i++] = 0x2b; /* dir  */

        /* SPI packet header */
        cmd_buf[i++] = 0x11; /* MC_DATA_OUT | MC_DATA_OCN */
        cmd_buf[i++] = (llen+1-1) & 0xff;
        cmd_buf[i++] = (llen+1-1) >> 8;

        /* SPI payload */
        cmd_buf[i++] = 0x80;

        Color tmpColor;
        for(const auto& screen : screens) {
            for(int x = 0; x < screen->getWidth(); x++) {
                switch (screen->getRotation()) {
                    case Rotation::rot0:
                        tmpColor = screen->getPixel(x, y);
                        break;
                    case Rotation::rot90:
                        tmpColor = screen->getPixel(screenHeight-1-y, x);
                        break;
                    case Rotation::rot180:
                        tmpColor = screen->getPixel(screenWidth-1-x, screenHeight-1-y);
                        break;
                    case Rotation::rot270:
                        tmpColor = screen->getPixel(y, screenWidth-1-x);
                        break;
                    default:
                        break;
                }
                if(bitDepthInBytes == 2){
                    uint16_t *pixelP = (uint16_t *)(cmd_buf + i + screen->getOffsetX() * (llen / screenCount) + x * bitDepthInBytes);
                    *pixelP = tmpColor.r() >> 3 << 11 | tmpColor.g() >> 2 << 6 | tmpColor.b() >> 3;
                }else if(bitDepthInBytes == 3){
                    cmd_buf[i + screen->getOffsetX() * (llen / screenCount) + x * bitDepthInBytes] = tmpColor.r();
                    cmd_buf[i + screen->getOffsetX() * (llen / screenCount) + x * bitDepthInBytes + 1] = tmpColor.g();
                    cmd_buf[i + screen->getOffsetX() * (llen / screenCount) + x * bitDepthInBytes + 2] = tmpColor.b();
                }
            }
        }
        i += llen;

        /* Set CS high */
        cmd_buf[i++] = 0x80; /* MC_SETB_LOW */
        cmd_buf[i++] = 0x28; /* gpio */
        cmd_buf[i++] = 0x2b; /* dir  */

        /* Set CS low */
        cmd_buf[i++] = 0x80; /* MC_SETB_LOW */
        cmd_buf[i++] = 0x00; /* gpio */
        cmd_buf[i++] = 0x2b; /* dir  */

        /* SPI header */
        cmd_buf[i++] = 0x11; /* MC_DATA_OUT | MC_DATA_OCN */
        cmd_buf[i++] = 2-1;
        cmd_buf[i++] = 0;

        /* SPI payload */
        cmd_buf[i++] = 0x03;
        cmd_buf[i++] = y;

        /* Set CS high */
        cmd_buf[i++] = 0x80; /* MC_SETB_LOW */
        cmd_buf[i++] = 0x28; /* gpio */
        cmd_buf[i++] = 0x2b; /* dir  */

        mpsse_send_raw(cmd_buf, i);
    }

    /* Swap Frame */
    cmd_buf[0] = 0x04;
    cmd_buf[1] = 0x00;
    set_cs(0);
    mpsse_send_spi(cmd_buf, 2);
    set_cs(1);

//    auto usTotal = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()) - usStart;
//    std::cout << "render: " << usTotal.count() << " us" << std::endl;

    renderMutex.unlock();
}

void FPGARendererFTDI::setGlobalBrightness(int brightness) {
    if (brightness <= 100 && brightness >= 0) {
        globalBrightness = brightness;
    }
}

int FPGARendererFTDI::getGlobalBrightness() {
    return globalBrightness;
}
