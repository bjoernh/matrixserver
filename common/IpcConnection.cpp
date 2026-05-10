#include "IpcConnection.h"

#include <boost/log/trivial.hpp>

IpcConnection::IpcConnection(){
    receiveCallback = NULL;
    receiveThread = nullptr;
    dead = false;
}

IpcConnection::IpcConnection(std::shared_ptr<boost::interprocess::message_queue> sender,
                              std::shared_ptr<boost::interprocess::message_queue> receiver,
                              std::string name) {
    sendMQ = sender;
    receiveMQ = receiver;
    mqName = name;
    receiveCallback = NULL;
    receiveThread = nullptr;
    dead = false;
}


void IpcConnection::startReceiving() {
    receiveThread = new boost::thread(&IpcConnection::readLoop, this);
}

void IpcConnection::setReceiveCallback(
        std::function<void(std::shared_ptr<UniversalConnection>,
                           std::shared_ptr<matrixserver::MatrixServerMessage>)> callback) {
    receiveCallback = callback;
    this->startReceiving();
}

#include <boost/date_time/posix_time/posix_time.hpp>

void IpcConnection::readLoop() {
    boost::interprocess::message_queue::size_type recvd_size;
    unsigned int priority;
    BOOST_LOG_TRIVIAL(trace) << "[IpcConnection] start read loop";
    while(!dead){
        try {
            boost::this_thread::interruption_point();
            boost::posix_time::ptime timeout = boost::posix_time::microsec_clock::universal_time() + boost::posix_time::milliseconds(100);
            if (this->receiveMQ->timed_receive(&receiveData, MAXIPCMESSAGESIZE, recvd_size, priority, timeout)) {
                auto receiveMessage = std::make_shared<matrixserver::MatrixServerMessage>();
                if (receiveMessage->ParseFromString(std::string(receiveData, recvd_size))) {
                    BOOST_LOG_TRIVIAL(trace) << "[IpcConnection] Recieved full Protobuf MatrixServerMessage";
                    if (this->receiveCallback != NULL) {
                        this->receiveCallback(shared_from_this(), receiveMessage);
                    }else{
                        BOOST_LOG_TRIVIAL(trace) << "[IpcConnection] NO CALLBACK!";
                    }
                }
            }
        } catch (const boost::interprocess::interprocess_exception &e) {
            BOOST_LOG_TRIVIAL(error) << "[IpcConnection] Receive error: " << e.what();
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
        BOOST_LOG_TRIVIAL(error) << "[IpcConnection] Message too large: "
                                 << sendBuffer.size() << " > " << MAXIPCMESSAGESIZE;
        return;
    }
    try {
        sendMQ->send(sendBuffer.data(), sendBuffer.size(), 0);
    } catch (const boost::interprocess::interprocess_exception &e) {
        BOOST_LOG_TRIVIAL(error) << "[IpcConnection] Send failed: " << e.what();
        setDead(true);
    }
}

bool IpcConnection::isDead() {
    return dead;
}

IpcConnection::~IpcConnection() {
    setDead(true);
    if (receiveThread) {
        receiveThread->interrupt();
        if (receiveThread->joinable()) {
            receiveThread->join();
        }
        delete receiveThread;
        receiveThread = nullptr;
    }
    if (!mqName.empty()) {
        BOOST_LOG_TRIVIAL(debug) << "[IpcConnection] Removing message queue: " << mqName;
        boost::interprocess::message_queue::remove(mqName.c_str());
    }
}

void IpcConnection::setDead(bool sDead) {
    dead = sDead;
}

bool IpcConnection::connectToServer(std::string serverAddress) {
    try {
        if (!mqName.empty()) {
            boost::interprocess::message_queue::remove(mqName.c_str());
        }
        std::stringstream receiveMQname;
        for(int i = 0; i < 20; i++)
            receiveMQname << (char)(rand()%26+'a'); // add random character [a...z]
        mqName = receiveMQname.str();

        auto tempServer = std::make_shared<boost::interprocess::message_queue>(boost::interprocess::open_only, serverAddress.data());
        this->receiveMQ = std::make_shared<boost::interprocess::message_queue>(boost::interprocess::open_or_create, mqName.data(), 10, MAXIPCMESSAGESIZE, boost::interprocess::permissions(0666));
        tempServer->send(mqName.data(), mqName.size(), 0);
        std::vector<char> tempData(MAXIPCMESSAGESIZE);
        boost::interprocess::message_queue::size_type recvd_size;
        unsigned int priority;
        this->receiveMQ->receive(tempData.data(), MAXIPCMESSAGESIZE, recvd_size, priority); //blocking
        if(recvd_size == 20){
            this->sendMQ = std::make_shared<boost::interprocess::message_queue>(boost::interprocess::open_only, std::string(tempData.data(), recvd_size).data());
            setDead(false);
        }else{
            setDead(true);
        }
    } catch (boost::interprocess::interprocess_exception e) {
        BOOST_LOG_TRIVIAL(debug) << "[IpcConnection] " << e.what();
        setDead(true);
        return false;
    }
    return true;
}

