#include "WebSocketSimulatorRenderer.h"

#include <boost/log/trivial.hpp>
#include <matrixserver.pb.h>

WebSocketSimulatorRenderer::WebSocketSimulatorRenderer(
    std::vector<std::shared_ptr<Screen>> initScreens, std::string setPort)
    : port(std::move(setPort)), ioContext() {

  IRenderer::screens = std::move(initScreens);

  // Set up the TCP acceptor
  auto const address = net::ip::make_address("0.0.0.0");
  auto const portNum = static_cast<unsigned short>(std::stoi(port));
  tcp::endpoint endpoint{address, portNum};

  acceptor = std::make_unique<tcp::acceptor>(ioContext, endpoint);
  acceptor->set_option(tcp::acceptor::reuse_address(true));

  BOOST_LOG_TRIVIAL(info) << "[WebSocketRenderer] Listening on WebSocket port "
                          << port;

  // Accept loop runs in a dedicated thread
  ioThread = boost::thread([this] { this->acceptLoop(); });
}

WebSocketSimulatorRenderer::~WebSocketSimulatorRenderer() {
  running = false;
  ioContext.stop();
  if (ioThread.joinable()) {
    ioThread.join();
  }
}

// ── Accept Loop ─────────────────────────────────────────────────────────────
void WebSocketSimulatorRenderer::acceptLoop() {
  BOOST_LOG_TRIVIAL(debug) << "[WebSocketRenderer] Accept loop started";

  while (running) {
    beast::error_code ec;
    tcp::socket socket(ioContext);
    acceptor->accept(socket, ec);

    if (ec) {
      if (running) {
        BOOST_LOG_TRIVIAL(debug)
            << "[WebSocketRenderer] Accept error: " << ec.message();
      }
      break;
    }

    BOOST_LOG_TRIVIAL(info) << "[WebSocketRenderer] Browser connected from "
                            << socket.remote_endpoint(ec).address().to_string();

    runSession(std::move(socket));
  }

  BOOST_LOG_TRIVIAL(debug) << "[WebSocketRenderer] Accept loop ended";
}

// ── WebSocket Session ────────────────────────────────────────────────────────
void WebSocketSimulatorRenderer::runSession(tcp::socket socket) {
  auto ws = std::make_shared<WsStream>(std::move(socket));

  beast::error_code ec;

  // Perform WebSocket handshake
  ws->accept(ec);
  if (ec) {
    BOOST_LOG_TRIVIAL(debug)
        << "[WebSocketRenderer] Handshake error: " << ec.message();
    return;
  }

  // Disable fragmentation and use binary mode
  ws->binary(true);
  ws->auto_fragment(false);

  {
    std::lock_guard<std::mutex> lock(sessionMutex);
    activeSession = ws;
  }

  BOOST_LOG_TRIVIAL(info) << "[WebSocketRenderer] WebSocket session active";

  // Drain incoming messages (ignore them in view-only Phase 1)
  beast::flat_buffer buf;
  while (running) {
    ws->read(buf, ec);
    if (ec) {
      if (ec != websocket::error::closed) {
        BOOST_LOG_TRIVIAL(debug)
            << "[WebSocketRenderer] Read error: " << ec.message();
      }
      break;
    }
    buf.consume(buf.size());
  }

  {
    std::lock_guard<std::mutex> lock(sessionMutex);
    activeSession.reset();
  }

  BOOST_LOG_TRIVIAL(info) << "[WebSocketRenderer] Browser disconnected";
}

// ── IRenderer interface ──────────────────────────────────────────────────────
void WebSocketSimulatorRenderer::setScreenData(int screenId, Color *data) {
  if (screenId < static_cast<int>(screens.size())) {
    screens.at(screenId)->setScreenData(data);
  }
}

void WebSocketSimulatorRenderer::render() {
  std::shared_ptr<WsStream> session;
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

  // Send as WebSocket binary message (no COBS — WebSocket handles framing)
  beast::error_code ec;
  session->write(net::buffer(serialised), ec);
  if (ec) {
    BOOST_LOG_TRIVIAL(debug)
        << "[WebSocketRenderer] Write error: " << ec.message();
    std::lock_guard<std::mutex> lock(sessionMutex);
    activeSession.reset();
  }
}

void WebSocketSimulatorRenderer::setGlobalBrightness(int brightness) {
  globalBrightness = brightness;
}

int WebSocketSimulatorRenderer::getGlobalBrightness() {
  return globalBrightness;
}
