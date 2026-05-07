#ifndef MATRIXSERVER_CONNECTIONFACTORY_H
#define MATRIXSERVER_CONNECTIONFACTORY_H

#include <memory>
#include <string>
#include <boost/asio.hpp>
#include <UniversalConnection.h>

/// Factory that parses a URI string and returns the appropriate
/// UniversalConnection instance.
///
/// Supported URI schemes:
///   ipc://<queue-name>          — IpcConnection (Boost message queue)
///   tcp://<host>:<port>         — TcpClient / SocketConnection
///   unix://<path>               — UnixSocketClient / SocketConnection
///
/// Returns nullptr and logs an error on parse failure or unknown scheme.
/// The caller is responsible for checking isDead() before using the returned
/// connection — this factory does NOT guarantee that the connection
/// succeeded; it only dispatches to the right concrete type.

namespace ConnectionFactory {

std::shared_ptr<UniversalConnection> connectFromUri(boost::asio::io_context& ctx, const std::string& uri);

} // namespace ConnectionFactory

#endif // MATRIXSERVER_CONNECTIONFACTORY_H
