#ifndef MATRIXSERVER_MESSAGEDISPATCHER_H
#define MATRIXSERVER_MESSAGEDISPATCHER_H

#include <functional>
#include <memory>
#include <unordered_map>

#include <UniversalConnection.h>
#include <matrixserver.pb.h>

/// Routes incoming MatrixServerMessages to registered handler functions by
/// MessageType.  Each handler is a free function, lambda, or bound member
/// function with the signature:
///
///   void(std::shared_ptr<UniversalConnection>,
///        std::shared_ptr<matrixserver::MatrixServerMessage>)
///
/// If no handler is registered for a given MessageType, the message is
/// silently dropped (matching the original switch default-case behaviour) and
/// a trace log line is emitted.

class MessageDispatcher {
  public:
    using Handler = std::function<void(std::shared_ptr<UniversalConnection>, std::shared_ptr<matrixserver::MatrixServerMessage>)>;

    /// Register a handler for the given message type.
    /// A second registration for the same type overwrites the first.
    void registerHandler(matrixserver::MessageType type, Handler h);

    /// Look up the handler for msg->messagetype() and invoke it.
    /// Unknown types are logged at trace level and dropped.
    void dispatch(std::shared_ptr<UniversalConnection> conn, std::shared_ptr<matrixserver::MatrixServerMessage> msg);

  private:
    std::unordered_map<int, Handler> handlers_;
};

#endif // MATRIXSERVER_MESSAGEDISPATCHER_H
