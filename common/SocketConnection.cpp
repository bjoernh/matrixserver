#include "SocketConnection.h"

#include <boost/log/trivial.hpp>

SocketConnection::SocketConnection(boost::asio::io_context &io_context) :
        io(io_context), socket(io), cobsDecoder(RECEIVE_BUFFER_SIZE) {
    receiveCallback = NULL;
}

boost::asio::generic::stream_protocol::socket &SocketConnection::getSocket() {
    return socket;
}


void SocketConnection::startReceiving() {
    this->doRead();
}

void SocketConnection::setReceiveCallback(
        std::function<void(std::shared_ptr<UniversalConnection>,
                           std::shared_ptr<matrixserver::MatrixServerMessage>)> callback) {
    receiveCallback = callback;
}

void SocketConnection::doRead() {
    BOOST_LOG_TRIVIAL(trace) << "[SOCK CON] Starting Read";
    socket.async_read_some(
            boost::asio::buffer(this->recv_buffer, RECEIVE_BUFFER_SIZE),
            [this](boost::system::error_code error, size_t bytes_transferred) {
                this->handleRead(error, bytes_transferred);
            });
}

void SocketConnection::handleRead(const boost::system::error_code &error, size_t bytes_transferred) {
    BOOST_LOG_TRIVIAL(trace) << "[SOCK CON] Handling Read";
    if (!error) {
        BOOST_LOG_TRIVIAL(trace) << "[SOCK CON] Received: " << bytes_transferred << " bytes";
        auto packets = cobsDecoder.insertBytesAndReturnDecodedPackets((uint8_t *) this->recv_buffer, bytes_transferred);
        for (auto packet : packets) {
            auto receiveMessage = std::make_shared<matrixserver::MatrixServerMessage>();
            if (receiveMessage->ParseFromString(packet)) {
                BOOST_LOG_TRIVIAL(trace) << "[SOCK CON] Recieved full Protobuf MatrixServerMessage";
                if (receiveCallback != NULL) {
                    receiveCallback(shared_from_this(), receiveMessage);
                }
            } else {
                if (message_buffer.size() > RECEIVE_BUFFER_SIZE) {
                    BOOST_LOG_TRIVIAL(debug) << "[SOCK CON] Message Buffer to big, resetting";
                }
            }
        }
        this->doRead();
    } else {
        BOOST_LOG_TRIVIAL(debug) << "[SOCK CON] Read Error: " << error.message();
        dead = true;
    }
}


void SocketConnection::sendMessage(std::shared_ptr<matrixserver::MatrixServerMessage> message) {
    auto sendBuffer = Cobs::encode(message->SerializeAsString());

    boost::asio::post(io, [this, self=shared_from_this(), sendBuffer=std::move(sendBuffer)]() mutable {
        if (write_queue.size() > 5) {
            BOOST_LOG_TRIVIAL(warning) << "[SOCK CON] Write queue full, dropping message";
            return;
        }
        bool empty = write_queue.empty();
        write_queue.push(std::move(sendBuffer));
        if (empty && !is_writing) {
            doWrite();
        }
    });
}

void SocketConnection::doWrite() {
    is_writing = true;
    boost::asio::async_write(socket,
                             boost::asio::buffer(write_queue.front().data(), write_queue.front().size()),
                             [this, self=shared_from_this()](boost::system::error_code error, size_t bytes_transferred) {
                                 this->handleWrite(error, bytes_transferred);
                             });
}

void SocketConnection::handleWrite(const boost::system::error_code &error, size_t bytes_transferred) {
    BOOST_LOG_TRIVIAL(trace) << "[SOCK CON] Handling Write";
    if (!error) {
        BOOST_LOG_TRIVIAL(trace) << "[SOCK CON] Written: " << bytes_transferred << " bytes";
    } else {
        BOOST_LOG_TRIVIAL(debug) << "[SOCK CON] Write Error: " << error.message();
        dead = true;
    }
    
    write_queue.pop();
    if (!write_queue.empty()) {
        doWrite();
    } else {
        is_writing = false;
    }
}

bool SocketConnection::isDead() {
    return dead;
}

SocketConnection::~SocketConnection() {
    boost::system::error_code ec;
    socket.close(ec);
}

void SocketConnection::setDead(bool sDead) {
    dead = sDead;
}
