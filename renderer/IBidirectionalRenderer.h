#ifndef MATRIXSERVER_IBIDIRECTIONALRENDERER_H
#define MATRIXSERVER_IBIDIRECTIONALRENDERER_H

#include <IRenderer.h>
#include <functional>
#include <memory>

namespace matrixserver {
class MatrixServerMessage;
}

/**
 * IBidirectionalRenderer — extends IRenderer with an optional messaging
 * channel for renderers that can both send and receive application messages
 * (e.g. the WebSocket simulator renderer).
 *
 * FPGA and RGB-Matrix renderers inherit from IRenderer only, since they
 * have no back-channel to clients.
 */
class IBidirectionalRenderer : public IRenderer {
public:
  using MsgCallback =
      std::function<void(std::shared_ptr<matrixserver::MatrixServerMessage>)>;

  /**
   * Register a callback that the renderer invokes when a message arrives
   * from the connected client (e.g. browser → server).
   */
  virtual void setClientMessageCallback(MsgCallback cb) = 0;

  /**
   * Send a message to the connected client.
   * No-op if no client is currently connected.
   */
  virtual void
  sendMessage(std::shared_ptr<matrixserver::MatrixServerMessage> msg) = 0;
};

#endif // MATRIXSERVER_IBIDIRECTIONALRENDERER_H
