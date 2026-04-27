#ifndef MATRIXSERVER_RENDERERREGISTRY_H
#define MATRIXSERVER_RENDERERREGISTRY_H

#include <IRenderer.h>
#include <IBidirectionalRenderer.h>
#include <Color.h>
#include <functional>
#include <memory>
#include <vector>

namespace matrixserver {
class MatrixServerMessage;
}

class RendererRegistry {
public:
    using MsgCallback = IBidirectionalRenderer::MsgCallback;

    void add(std::shared_ptr<IRenderer> r);
    void renderAll();
    void setScreenData(int sid, Color* data);
    void setBrightness(int b);
    void broadcastMessage(std::shared_ptr<matrixserver::MatrixServerMessage> msg);
    void setMessageCallback(MsgCallback cb);
    void forEachRenderer(std::function<void(std::shared_ptr<IRenderer>)> fn);

private:
    std::vector<std::shared_ptr<IRenderer>> renderers_;
    std::vector<std::shared_ptr<IBidirectionalRenderer>> biDirRenderers_;
    MsgCallback messageCallback_;
};

#endif //MATRIXSERVER_RENDERERREGISTRY_H
