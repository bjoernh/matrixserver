#ifndef MATRIXSERVER_FPGARENDERERFTDI_H
#define MATRIXSERVER_FPGARENDERERFTDI_H

#include <IRenderer.h>
#include <mutex>
#include <vector>
#include <cstdint>
#include "Screen.h"

class FPGARendererFTDI : public IRenderer {
public:
    FPGARendererFTDI();

    FPGARendererFTDI(std::vector<std::shared_ptr<Screen>>);

    void init(std::vector<std::shared_ptr<Screen>>);

    void setScreenData(int, Color *);

    void render();

    void setGlobalBrightness(int);

    int getGlobalBrightness();

private:
    std::vector<std::shared_ptr<Screen>> screens;
    std::mutex renderMutex;
    // Reusable per-frame buffers (allocated once in init, reused each render).
    std::vector<uint8_t> frameBuf_;  // flen bytes
    std::vector<uint8_t> cmdBuf_;   // llen+128 bytes
};

#endif //MATRIXSERVER_FPGARENDERERFTDI_H
