#include "TcpServer.h"
#include <boost/log/trivial.hpp>
#include <fcntl.h>

TcpServer::TcpServer(boost::asio::io_context &setIo, boost::asio::ip::tcp::endpoint setEndpoint) : io(setIo), endpoint(setEndpoint), acceptor(setIo) {
    acceptCallback = nullptr;
    acceptor.open(endpoint.protocol());
    // Prevent child processes (launched via system()) from inheriting this socket,
    // which would keep the port bound after the server exits.
    ::fcntl(acceptor.native_handle(), F_SETFD, ::fcntl(acceptor.native_handle(), F_GETFD) | FD_CLOEXEC);
    acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    acceptor.bind(endpoint);
    acceptor.listen();
    this->doAccept();
}

void TcpServer::doAccept() {
    BOOST_LOG_TRIVIAL(debug) << "[Server] Start accepting on address: " << this->endpoint.address().to_string()
                             << " and port: " << this->endpoint.port();
    auto tempCon = std::make_shared<SocketConnection>(io);
    this->acceptor.async_accept(tempCon->getSocket(), [this, tempCon](boost::system::error_code error) { this->handleAccept(error, tempCon); });
}

void TcpServer::handleAccept(const boost::system::error_code &error, std::shared_ptr<SocketConnection> connection) {
    if (!error) {
        BOOST_LOG_TRIVIAL(debug) << "[Server] Accepted Connection";
        connection->startReceiving();
        if (acceptCallback != nullptr) {
            acceptCallback(connection);
        }
        doAccept();
    } else {
        BOOST_LOG_TRIVIAL(debug) << "[Server] Server Accept Error: " << error.message();
    }
}

void TcpServer::setAcceptCallback(std::function<void(std::shared_ptr<SocketConnection>)> callback) { acceptCallback = callback; }
