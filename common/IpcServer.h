#ifndef MATRIXSERVER_IPCSERVER_H
#define MATRIXSERVER_IPCSERVER_H

#include <thread>
#include <atomic>
#include <vector>
#include "IpcConnection.h"

class IpcServer {
public:
    IpcServer(std::string serverAddress);
    ~IpcServer();

    void startAccepting();

    void setAcceptCallback(std::function<void(std::shared_ptr<UniversalConnection>)> callback);

private:
    void acceptLoop();
    std::shared_ptr<boost::interprocess::message_queue> serverMQ;
    std::function<void(std::shared_ptr<UniversalConnection>)> acceptCallback;
    std::unique_ptr<std::thread> acceptThread;
    std::atomic<bool> running_{false};
    std::vector<uint8_t> receiveBuffer;
};

#endif //MATRIXSERVER_IPCSERVER_H
