#include "IpcServer.h"
#include <boost/log/trivial.hpp>



IpcServer::IpcServer(std::string serverAddress)
    : serverName(serverAddress)
{
    boost::interprocess::message_queue::remove(serverName.data());
    serverMQ = std::make_shared<boost::interprocess::message_queue>(boost::interprocess::open_or_create, serverName.data(), 10, SERVERMESSAGESIZE, boost::interprocess::permissions(0666));
    acceptCallback = NULL;
    startAccepting();
    BOOST_LOG_TRIVIAL(debug) << "[Server] Start accepting on IPC Channel: " << serverName;
}

IpcServer::~IpcServer() {
    if (acceptThread) {
        acceptThread->interrupt();
        if (acceptThread->joinable()) {
            acceptThread->join();
        }
        delete acceptThread;
        acceptThread = nullptr;
    }
    boost::interprocess::message_queue::remove(serverName.data());
}

void IpcServer::startAccepting() {
    acceptThread = new boost::thread(&IpcServer::acceptLoop, this);
}

void IpcServer::acceptLoop() {
    while(1){
        try {
            boost::this_thread::interruption_point();
            boost::interprocess::message_queue::size_type recvd_size;
            unsigned int priority;
            boost::posix_time::ptime timeout = boost::posix_time::microsec_clock::universal_time() + boost::posix_time::milliseconds(500);
            if (this->serverMQ->timed_receive(&receiveBuffer, SERVERMESSAGESIZE, recvd_size, priority, timeout)) {
                std::stringstream sendMQname;
                for(int i = 0; i < 20; i++)
                    sendMQname << (char)(rand()%26+'a');

                auto receiveMQ = std::make_shared<boost::interprocess::message_queue>(boost::interprocess::open_or_create, sendMQname.str().data(), 10, SERVERMESSAGESIZE, boost::interprocess::permissions(0666));
                auto sendMQ = std::make_shared<boost::interprocess::message_queue>(boost::interprocess::open_only, std::string(receiveBuffer, recvd_size).data());
                sendMQ->send(sendMQname.str().data(), sendMQname.str().size(), 0);

                BOOST_LOG_TRIVIAL(debug) << "[Server] Accepted Connection, sendMQ " << receiveBuffer << " receiveMQ " << sendMQname.str();

                auto connection = std::make_shared<IpcConnection>(sendMQ, receiveMQ, sendMQname.str());
                if (acceptCallback != NULL) {
                    acceptCallback(connection);
                    connection->startReceiving();
                }
            }
        } catch (const boost::thread_interrupted &) {
            break;
        } catch (const boost::interprocess::interprocess_exception &e) {
            BOOST_LOG_TRIVIAL(error) << "[IpcServer] Accept error: " << e.what();
            continue;
        }
    }
}

void IpcServer::setAcceptCallback(std::function<void(std::shared_ptr<UniversalConnection>)> callback) {
    acceptCallback = callback;
}
