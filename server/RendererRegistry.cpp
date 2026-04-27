#include "RendererRegistry.h"
#include <matrixserver.pb.h>

void RendererRegistry::add(std::shared_ptr<IRenderer> r) {
    renderers_.push_back(r);
    if (auto biDir = std::dynamic_pointer_cast<IBidirectionalRenderer>(r)) {
        biDirRenderers_.push_back(biDir);
        if (messageCallback_) {
            biDir->setClientMessageCallback(messageCallback_);
        }
    }
}

void RendererRegistry::renderAll() {
    for (const auto& renderer : renderers_) {
        renderer->render();
    }
}

void RendererRegistry::setScreenData(int sid, Color* data) {
    for (const auto& renderer : renderers_) {
        renderer->setScreenData(sid, data);
    }
}

void RendererRegistry::setBrightness(int b) {
    for (const auto& renderer : renderers_) {
        renderer->setGlobalBrightness(b);
    }
}

void RendererRegistry::broadcastMessage(
    std::shared_ptr<matrixserver::MatrixServerMessage> msg) {
    for (const auto& biDir : biDirRenderers_) {
        biDir->sendMessage(msg);
    }
}

void RendererRegistry::setMessageCallback(MsgCallback cb) {
    messageCallback_ = std::move(cb);
}

void RendererRegistry::forEachRenderer(
    std::function<void(std::shared_ptr<IRenderer>)> fn) {
    for (const auto& renderer : renderers_) {
        fn(renderer);
    }
}
