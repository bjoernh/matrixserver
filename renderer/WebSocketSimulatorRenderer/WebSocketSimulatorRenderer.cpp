#include "WebSocketSimulatorRenderer.h"
#include "WsSession.h"
#include <boost/log/trivial.hpp>
#include <fcntl.h>
#include <matrixserver.pb.h>

WebSocketSimulatorRenderer::WebSocketSimulatorRenderer(
    std::vector<std::shared_ptr<Screen>> initScreens, std::string setPort,
    bool setStreamPixels)
    : port(std::move(setPort)), ioContext(), streamPixels(setStreamPixels) {

  IRenderer::screens = std::move(initScreens);

  // Set up the TCP acceptor
  auto const address = net::ip::make_address("0.0.0.0");
  auto const portNum = static_cast<unsigned short>(std::stoi(port));
  tcp::endpoint endpoint{address, portNum};

  acceptor = std::make_unique<tcp::acceptor>(ioContext);
  acceptor->open(endpoint.protocol());
  // Prevent child processes (launched via system()) from inheriting this socket.
  ::fcntl(acceptor->native_handle(), F_SETFD,
          ::fcntl(acceptor->native_handle(), F_GETFD) | FD_CLOEXEC);
  acceptor->set_option(tcp::acceptor::reuse_address(true));
  acceptor->bind(endpoint);
  acceptor->listen();

  BOOST_LOG_TRIVIAL(info) << "[WebSocketRenderer] Listening on WebSocket port "
                          << port;

  // Start async accept
  do_accept();
  // Accept loop runs in a dedicated thread
  ioThread = std::make_unique<std::thread>([this] { this->ioContext.run(); });
}

WebSocketSimulatorRenderer::~WebSocketSimulatorRenderer() {
  running = false;
  ioContext.stop();
  if (ioThread && ioThread->joinable()) {
    ioThread->join();
  }
}

void WebSocketSimulatorRenderer::setClientMessageCallback(MsgCallback cb) {
  clientMessageCb = cb;
}

void WebSocketSimulatorRenderer::do_accept() {
  if (!running)
    return;
  acceptor->async_accept([this](beast::error_code ec, tcp::socket socket) {
    if (!ec) {
      BOOST_LOG_TRIVIAL(info) << "[WebSocketRenderer] Browser connected";
      auto session = std::make_shared<WsSession>(
          std::move(socket), clientMessageCb, [this]() {
            std::lock_guard<std::mutex> lock(sessionMutex);
            activeSession.reset();
          });
      {
        std::lock_guard<std::mutex> lock(sessionMutex);
        activeSession = session;
      }
      session->run();
    } else {
      BOOST_LOG_TRIVIAL(debug)
          << "[WebSocketRenderer] Accept error: " << ec.message();
    }
    if (running)
      do_accept();
  });
}

// ── IRenderer interface ──────────────────────────────────────────────────────
void WebSocketSimulatorRenderer::setScreenData(int screenId, Color *data) {
  if (!streamPixels) return;
  if (screenId < static_cast<int>(screens.size())) {
    screens.at(screenId)->setScreenData(data);
  }
}

void WebSocketSimulatorRenderer::render() {
  if (!streamPixels) return;
  std::shared_ptr<WsSession> session;
  {
    std::lock_guard<std::mutex> lock(sessionMutex);
    session = activeSession;
  }

  if (!session) {
    return; // No browser connected — frame is silently dropped
  }

  // Build MatrixServerMessage with all screen data
  matrixserver::MatrixServerMessage message;
  message.set_messagetype(matrixserver::setScreenFrame);

  for (auto &screen : screens) {
    auto *screenData = message.add_screendata();
    screenData->set_screenid(screen->getScreenId());
    screenData->set_framedata(
        reinterpret_cast<const char *>(screen->getScreenData().data()),
        screen->getScreenDataSize() * sizeof(Color));
    screenData->set_encoding(matrixserver::ScreenData_Encoding_rgb24bbp);
  }

  // Serialise to binary protobuf
  std::string serialised;
  if (!message.SerializeToString(&serialised)) {
    BOOST_LOG_TRIVIAL(debug)
        << "[WebSocketRenderer] Failed to serialise protobuf message";
    return;
  }

  // Send as WebSocket binary message via queue
  session->send_msg(serialised, true);
}

void WebSocketSimulatorRenderer::setGlobalBrightness(int brightness) {
  globalBrightness = brightness;
}

int WebSocketSimulatorRenderer::getGlobalBrightness() {
  return globalBrightness;
}

void WebSocketSimulatorRenderer::sendMessage(std::shared_ptr<matrixserver::MatrixServerMessage> msg) {
  std::shared_ptr<WsSession> session;
  {
    std::lock_guard<std::mutex> lock(sessionMutex);
    session = activeSession;
  }

  if (!session) {
    return;
  }

  std::string serialised;
  if (msg->SerializeToString(&serialised)) {
    session->send_msg(serialised, false);
  }
}
