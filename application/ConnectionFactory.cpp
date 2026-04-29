#include "ConnectionFactory.h"

#include <IpcConnection.h>
#include <TcpClient.h>
#include <UnixSocketClient.h>

#include <boost/log/trivial.hpp>
#include <format>

namespace ConnectionFactory {

std::shared_ptr<UniversalConnection> connectFromUri(boost::asio::io_context& ctx, const std::string& uri) {
    // Parse URI scheme: everything before "://"
    const auto schemeEnd = uri.find("://");
    if (schemeEnd == std::string::npos) {
        BOOST_LOG_TRIVIAL(error) << std::format("[ConnectionFactory] Invalid URI format (no '://'): {}", uri);
        return nullptr;
    }

    const std::string scheme = uri.substr(0, schemeEnd);
    const std::string rest = uri.substr(schemeEnd + 3);

    if (scheme == "ipc") {
        // ipc://<queue-name>
        auto ipcCon = std::make_shared<IpcConnection>();
        ipcCon->connectToServer(rest);
        return ipcCon;

    } else if (scheme == "tcp") {
        // tcp://<host>:<port>
        const auto colonPos = rest.rfind(':');
        if (colonPos == std::string::npos) {
            BOOST_LOG_TRIVIAL(error) << std::format("[ConnectionFactory] Invalid TCP URI, expected tcp://<host>:<port>: {}", uri);
            return nullptr;
        }
        const std::string host = rest.substr(0, colonPos);
        const std::string port = rest.substr(colonPos + 1);
        return TcpClient::connect(ctx, host, port);

    } else if (scheme == "unix") {
        // unix:///tmp/<path>.socket  — rest holds the full path
        return UnixSocketClient::connect(ctx, rest);

    } else {
        BOOST_LOG_TRIVIAL(error) << std::format("[ConnectionFactory] Unknown URI scheme '{}' in: {}", scheme, uri);
        return nullptr;
    }
}

} // namespace ConnectionFactory
