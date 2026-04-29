#include "IpcConnection.h"

#include <format>
#include <boost/log/trivial.hpp>

IpcConnection::IpcConnection() : receiveData(MAXIPCMESSAGESIZE) { receiveCallback = nullptr; }

IpcConnection::IpcConnection(std::shared_ptr<boost::interprocess::message_queue> sender, std::shared_ptr<boost::interprocess::message_queue> receiver)
    : receiveData(MAXIPCMESSAGESIZE) {
    sendMQ = sender;
    receiveMQ = receiver;
    receiveCallback = nullptr;
    dead.store(false);
}

void IpcConnection::startReceiving() { receiveThread = std::make_unique<std::thread>(&IpcConnection::readLoop, this); }

void IpcConnection::setReceiveCallback(
    std::function<void(std::shared_ptr<UniversalConnection>, std::shared_ptr<matrixserver::MatrixServerMessage>)> callback) {
    receiveCallback = callback;
    this->startReceiving();
}

void IpcConnection::readLoop() {
    boost::interprocess::message_queue::size_type recvd_size;
    unsigned int priority;
    BOOST_LOG_TRIVIAL(trace) << std::format("[IpcConnection] start read loop");
    while (!dead.load()) {
        try {
            this->receiveMQ->receive(receiveData.data(), receiveData.size(), recvd_size, priority);
            if (dead.load())
                break;
            auto receiveMessage = std::make_shared<matrixserver::MatrixServerMessage>();
            if (receiveMessage->ParseFromString(std::string(reinterpret_cast<const char*>(receiveData.data()), recvd_size))) {
                BOOST_LOG_TRIVIAL(trace) << std::format("[IpcConnection] Recieved full Protobuf MatrixServerMessage");
                if (this->receiveCallback != nullptr) {
                    this->receiveCallback(shared_from_this(), receiveMessage);
                } else {
                    BOOST_LOG_TRIVIAL(trace) << std::format("[IpcConnection] NO CALLBACK!");
                }
            }
        } catch (const boost::interprocess::interprocess_exception& e) {
            BOOST_LOG_TRIVIAL(error) << std::format("[IpcConnection] Receive error: {}", e.what());
            setDead(true);
            break;
        }
    }
}

void IpcConnection::doRead() {
    //    std::thread([this](){
    //        char tempData[MAXIPCMESSAGESIZE];
    //        boost::interprocess::message_queue::size_type recvd_size;
    //        unsigned int priority;
    //        this->receiveMQ->receive(&tempData, MAXIPCMESSAGESIZE, recvd_size, priority); //blocking
    //        BOOST_LOG_TRIVIAL(debug) << "[IpcConnection] Recieved something";
    //        auto receiveMessage = std::make_shared<matrixserver::MatrixServerMessage>();
    //        if (receiveMessage->ParseFromString(std::string(tempData, recvd_size))) {
    //            BOOST_LOG_TRIVIAL(trace) << "[IpcConnection] Recieved full Protobuf MatrixServerMessage";
    //            if (this->receiveCallback != NULL) {
    //                this->receiveCallback(shared_from_this(), receiveMessage);
    //            }
    //        }
    //        this->doRead();
    //    }).detach();
}

void IpcConnection::sendMessage(std::shared_ptr<matrixserver::MatrixServerMessage> message) {
    auto sendBuffer = message->SerializeAsString();
    if (sendBuffer.size() > MAXIPCMESSAGESIZE) {
        BOOST_LOG_TRIVIAL(error) << std::format("[IpcConnection] Message too large: {} > {}", sendBuffer.size(), MAXIPCMESSAGESIZE);
        return;
    }
    try {
        sendMQ->send(sendBuffer.data(), sendBuffer.size(), 0);
    } catch (const boost::interprocess::interprocess_exception& e) {
        BOOST_LOG_TRIVIAL(error) << std::format("[IpcConnection] Send failed: {}", e.what());
        setDead(true);
    }
}

bool IpcConnection::isDead() { return dead.load(); }

IpcConnection::~IpcConnection() {
    // Signal readLoop to exit and unblock its blocking receive call.
    dead.store(true);
    if (receiveMQ) {
        try {
            receiveMQ->send("", 0, 0);
        } catch (...) {
        }
    }
    if (receiveThread && receiveThread->joinable()) {
        receiveThread->join();
    }
}

void IpcConnection::setDead(bool sDead) { dead.store(sDead); }

bool IpcConnection::connectToServer(std::string serverAddress) {
    try {
        std::string receiveMQname(20, ' ');
        for (int i = 0; i < 20; i++)
            receiveMQname[i] = (char)(rand() % 26 + 'a');

        auto tempServer = std::make_shared<boost::interprocess::message_queue>(boost::interprocess::open_only, serverAddress.data());
        this->receiveMQ = std::make_shared<boost::interprocess::message_queue>(boost::interprocess::open_or_create, receiveMQname.data(), 10,
                                                                               MAXIPCMESSAGESIZE, boost::interprocess::permissions(0666));
        tempServer->send(receiveMQname.data(), receiveMQname.size(), 0);
        std::vector<uint8_t> tempData(MAXIPCMESSAGESIZE);
        boost::interprocess::message_queue::size_type recvd_size;
        unsigned int priority;
        this->receiveMQ->receive(tempData.data(), tempData.size(), recvd_size, priority); // blocking
        if (recvd_size == 20) {
            this->sendMQ = std::make_shared<boost::interprocess::message_queue>(
                boost::interprocess::open_only, std::string(reinterpret_cast<const char*>(tempData.data()), recvd_size).data());
            setDead(false);
        } else {
            setDead(true);
        }
    } catch (boost::interprocess::interprocess_exception e) {
        BOOST_LOG_TRIVIAL(debug) << std::format("[IpcConnection] {}", e.what());
        setDead(true);
        return false;
    }
    return true;
}
