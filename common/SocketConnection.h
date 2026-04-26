#ifndef MATRIXSERVER_SOCKETCONNECTION_H
#define MATRIXSERVER_SOCKETCONNECTION_H

#include <boost/asio.hpp>
#include <functional>
#include <string>
#include <mutex>
#include <queue>
#include "Cobs.h"
#include <matrixserver.pb.h>
#include "UniversalConnection.h"

inline constexpr int RECEIVE_BUFFER_SIZE = 200000;

class SocketConnection :  public std::enable_shared_from_this<SocketConnection>, public UniversalConnection {
public:
    SocketConnection(boost::asio::io_context &io_context);

    ~SocketConnection();

    boost::asio::generic::stream_protocol::socket &getSocket();

    void startReceiving() override;

    void
    setReceiveCallback(std::function<void(std::shared_ptr<UniversalConnection>,
                                          std::shared_ptr<matrixserver::MatrixServerMessage>)> callback) override;

    void sendMessage(std::shared_ptr<matrixserver::MatrixServerMessage> message) override;

    bool isDead() override;

    void setDead(bool sDead) override;

private:
    void doRead();

    void doWrite();

    void handleWrite(const boost::system::error_code &error, size_t bytes_transferred);

    void handleRead(const boost::system::error_code &error, size_t bytes_transferred);

    boost::asio::io_context &io;
    boost::asio::generic::stream_protocol::socket socket;
    char recv_buffer[RECEIVE_BUFFER_SIZE];
    std::string message_buffer;
    std::queue<std::string> write_queue;
    bool is_writing = false;
    Cobs cobsDecoder;
    std::function<void(std::shared_ptr<UniversalConnection>,
                       std::shared_ptr<matrixserver::MatrixServerMessage>)> receiveCallback;
    bool dead = false;
};


#endif //MATRIXSERVER_SOCKETCONNECTION_H
