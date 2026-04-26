#include "IpcServer.h"
#include <boost/log/trivial.hpp>



IpcServer::IpcServer(std::string serverAddress)
    : receiveBuffer(SERVERMESSAGESIZE)
{
    boost::interprocess::message_queue::remove(serverAddress.data());
    serverMQ = std::make_shared<boost::interprocess::message_queue>(boost::interprocess::open_or_create, serverAddress.data(), 10, SERVERMESSAGESIZE, boost::interprocess::permissions(0666));
    acceptCallback = nullptr;
    startAccepting();
    BOOST_LOG_TRIVIAL(debug) << "[Server] Start accepting on IPC Channel: " << serverAddress;
}

IpcServer::~IpcServer() {
    running_.store(false);
    // Unblock the blocking serverMQ->receive() by sending a zero-byte dummy
    // message on the server queue so the acceptLoop wakes up and checks running_.
    try {
        serverMQ->send("", 0, 0);
    } catch (...) {}
    if (acceptThread && acceptThread->joinable()) {
        acceptThread->join();
    }
}

void IpcServer::startAccepting() {
    running_.store(true);
    acceptThread = std::make_unique<std::thread>(&IpcServer::acceptLoop, this);
}

void IpcServer::acceptLoop() {
    while(running_.load()){
        try {
            boost::interprocess::message_queue::size_type recvd_size;
            unsigned int priority;
            this->serverMQ->receive(receiveBuffer.data(), receiveBuffer.size(), recvd_size, priority);
            if (!running_.load())
                break;

            std::stringstream sendMQname;
            for(int i = 0; i < 20; i++)
                sendMQname << (char)(rand()%26+'a');

            auto receiveMQ = std::make_shared<boost::interprocess::message_queue>(boost::interprocess::open_or_create, sendMQname.str().data(), 10, SERVERMESSAGESIZE, boost::interprocess::permissions(0666));
            auto sendMQ = std::make_shared<boost::interprocess::message_queue>(boost::interprocess::open_only, std::string(reinterpret_cast<const char*>(receiveBuffer.data()), recvd_size).data());
            sendMQ->send(sendMQname.str().data(), sendMQname.str().size(), 0);

            BOOST_LOG_TRIVIAL(debug) << "[Server] Accepted Connection, sendMQ " << std::string(reinterpret_cast<const char*>(receiveBuffer.data()), recvd_size) << " receiveMQ " << sendMQname.str();

            auto connection = std::make_shared<IpcConnection>(sendMQ, receiveMQ);
            if (acceptCallback != nullptr) {
                acceptCallback(connection);
                connection->startReceiving();
            }
        } catch (const boost::interprocess::interprocess_exception &e) {
            BOOST_LOG_TRIVIAL(error) << "[IpcServer] Accept error: " << e.what();
            continue;
        }
    }
}

void IpcServer::setAcceptCallback(std::function<void(std::shared_ptr<UniversalConnection>)> callback) {
    acceptCallback = callback;
}
