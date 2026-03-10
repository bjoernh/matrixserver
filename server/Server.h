#ifndef MATRIXSERVER_SERVER_H
#define MATRIXSERVER_SERVER_H

#include <vector>
#include <memory>
#include <mutex>
#include <boost/thread/thread.hpp>

#include <Screen.h>
#include <App.h>
#include <IRenderer.h>
#include <matrixserver.pb.h>
#include <TcpServer.h>
#include <UnixSocketServer.h>
#include <IpcServer.h>
#include <Joystick.h>

class Server {
public:
    Server() = default;

    ~Server() = default;

    Server(std::shared_ptr<IRenderer>, matrixserver::ServerConfig &);

    bool tick();

    void handleRequest(std::shared_ptr<UniversalConnection> connection, std::shared_ptr<matrixserver::MatrixServerMessage> message);

    void newConnectionCallback(std::shared_ptr<UniversalConnection>);

    void addRenderer(std::shared_ptr<IRenderer>);

    App * getAppByID(int searchID);

private:
    std::mutex appsMutex;
    std::vector<App> apps;
    std::vector<std::shared_ptr<IRenderer>> renderers;
    boost::asio::io_service ioContext;
    std::unique_ptr<boost::asio::io_service::work> ioWork;
    matrixserver::ServerConfig & serverConfig;
    TcpServer tcpServer;
//    UnixSocketServer unixServer;
    IpcServer ipcServer;
    boost::thread *ioThread;
    std::vector<std::shared_ptr<UniversalConnection>> connections;
    JoystickManager joystickmngr;
};


#endif //MATRIXSERVER_SERVER_H
