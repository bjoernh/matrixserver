#include "MessageDispatcher.h"

#include <boost/log/trivial.hpp>

void MessageDispatcher::registerHandler(matrixserver::MessageType type, Handler h) { handlers_[static_cast<int>(type)] = std::move(h); }

void MessageDispatcher::dispatch(std::shared_ptr<UniversalConnection> conn, std::shared_ptr<matrixserver::MatrixServerMessage> msg) {
    const auto it = handlers_.find(static_cast<int>(msg->messagetype()));
    if (it == handlers_.end()) {
        BOOST_LOG_TRIVIAL(trace) << "[MessageDispatcher] No handler for message type " << msg->messagetype() << " — dropping";
        return;
    }
    it->second(std::move(conn), std::move(msg));
}
