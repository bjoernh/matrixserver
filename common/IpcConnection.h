#ifndef MATRIXSERVER_IPCCONNECTION_H
#define MATRIXSERVER_IPCCONNECTION_H

#include <boost/interprocess/ipc/message_queue.hpp>
#include <thread>
#include <atomic>
#include <functional>
#include <string>
#include <mutex>
#include <vector>
#include <matrixserver.pb.h>
#include "UniversalConnection.h"

inline constexpr int SERVERMESSAGESIZE = 1000000;
inline constexpr int MAXIPCMESSAGESIZE = 1000000;

class IpcConnection :  public std::enable_shared_from_this<IpcConnection>, public UniversalConnection {
public:
    IpcConnection();

    IpcConnection(std::shared_ptr<boost::interprocess::message_queue> sender, std::shared_ptr<boost::interprocess::message_queue> receiver);

    ~IpcConnection();

    bool connectToServer(std::string serverAddress);

    void startReceiving() override;

    void
    setReceiveCallback(std::function<void(std::shared_ptr<UniversalConnection>,
                                          std::shared_ptr<matrixserver::MatrixServerMessage>)> callback) override;

    void sendMessage(std::shared_ptr<matrixserver::MatrixServerMessage> message) override;

    bool isDead() override;

    void setDead(bool sDead) override;
private:
    void doRead();

    void readLoop();

    std::shared_ptr<boost::interprocess::message_queue> sendMQ;
    std::shared_ptr<boost::interprocess::message_queue> receiveMQ;

    std::unique_ptr<std::thread> receiveThread;

    std::mutex sendMutex;
    std::string message_buffer;
    std::function<void(std::shared_ptr<UniversalConnection>,
                       std::shared_ptr<matrixserver::MatrixServerMessage>)> receiveCallback;
    std::atomic<bool> dead{false};

    std::vector<uint8_t> receiveData;
};


#endif //MATRIXSERVER_IPCCONNECTION_H
