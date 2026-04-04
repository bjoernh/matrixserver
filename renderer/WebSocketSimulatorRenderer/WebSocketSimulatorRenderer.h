#ifndef MATRIXSERVER_WEBSOCKETSIMULATORRENDERERER_H
#define MATRIXSERVER_WEBSOCKETSIMULATORRENDERERER_H

#include <IRenderer.h>
#include <Screen.h>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/thread/thread.hpp>
#include <memory>
#include <string>
#include <vector>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

#define WS_SIMULATOR_DEFAULT_PORT "1337"

/**
 * WebSocketSimulatorRenderer
 *
 * Implements IRenderer by opening a Boost.Beast WebSocket server on the
 * configured port. The browser-based CubeSimulator connects to this server.
 *
 * Protocol:
 *   - Accepts one connection at a time.
 *   - On render(), serialises the MatrixServerMessage to raw protobuf binary
 *     and sends it as a WebSocket binary message (no COBS framing needed).
 *   - Reconnects automatically if the browser disconnects.
 */
class WebSocketSimulatorRenderer : public IRenderer {
public:
  explicit WebSocketSimulatorRenderer(
      std::vector<std::shared_ptr<Screen>> screens,
      std::string port = WS_SIMULATOR_DEFAULT_PORT,
      bool streamPixels = true);

  ~WebSocketSimulatorRenderer();

  void setScreenData(int screenId, Color *data) override;

  void render() override;

  void setGlobalBrightness(int brightness) override;

  int getGlobalBrightness() override;

  void sendMessage(std::shared_ptr<matrixserver::MatrixServerMessage> msg) override;

  void setClientMessageCallback(
      std::function<void(std::shared_ptr<matrixserver::MatrixServerMessage>)>
          cb) override;

private:
  void do_accept();

  std::string port;
  net::io_context ioContext;
  std::unique_ptr<tcp::acceptor> acceptor;

  // Active WebSocket session (nullable — no client connected yet)
  std::shared_ptr<class WsSession> activeSession;
  std::mutex sessionMutex;

  boost::thread ioThread;
  bool running = true;

  std::function<void(std::shared_ptr<matrixserver::MatrixServerMessage>)>
      clientMessageCb;

  bool streamPixels;
};

#endif // MATRIXSERVER_WEBSOCKETSIMULATORRENDERERER_H
