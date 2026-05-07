#ifndef WS_SESSION_H
#define WS_SESSION_H

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/log/trivial.hpp>
#include <iostream>
#include <matrixserver.pb.h>
#include <memory>
#include <mutex>
#include <queue>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class WsSession : public std::enable_shared_from_this<WsSession> {
    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer buffer_;
    std::queue<std::string> write_queue_;
    bool is_writing_ = false;
    std::function<void(std::shared_ptr<matrixserver::MatrixServerMessage>)> cb_;
    std::function<void()> on_close_;

  public:
    WsSession(tcp::socket socket, std::function<void(std::shared_ptr<matrixserver::MatrixServerMessage>)> cb, std::function<void()> on_close)
        : ws_(std::move(socket)), cb_(std::move(cb)), on_close_(std::move(on_close)) {}

    void run() {
        ws_.binary(true);
        ws_.auto_fragment(false);
        ws_.async_accept(beast::bind_front_handler(&WsSession::on_accept, shared_from_this()));
    }

    void on_accept(beast::error_code ec) {
        if (ec) {
            on_close_();
            return;
        }
        do_read();
    }

    void do_read() { ws_.async_read(buffer_, beast::bind_front_handler(&WsSession::on_read, shared_from_this())); }

    void on_read(beast::error_code ec, std::size_t bytes) {
        if (ec) {
            on_close_();
            return;
        }
        if (cb_ && buffer_.size() > 0) {
            auto msg = std::make_shared<matrixserver::MatrixServerMessage>();
            if (msg->ParseFromArray(buffer_.data().data(), buffer_.size())) {
                cb_(msg);
            }
        }
        buffer_.consume(buffer_.size());
        do_read();
    }

    void send_msg(std::string msg, bool droppable = true) {
        net::post(ws_.get_executor(), [self = shared_from_this(), msg = std::move(msg), droppable]() mutable {
            if (droppable && self->write_queue_.size() > 2) {
                return; // Drop frames to prevent latency buildup and memory issues
            }
            bool empty = self->write_queue_.empty();
            self->write_queue_.push(std::move(msg));
            if (empty && !self->is_writing_) {
                self->do_write();
            }
        });
    }

    void do_write() {
        is_writing_ = true;
        ws_.async_write(net::buffer(write_queue_.front()), beast::bind_front_handler(&WsSession::on_write, shared_from_this()));
    }

    void on_write(beast::error_code ec, std::size_t bytes) {
        if (ec) {
            on_close_();
            return;
        }
        write_queue_.pop();
        if (!write_queue_.empty()) {
            do_write();
        } else {
            is_writing_ = false;
        }
    }
};

#endif
