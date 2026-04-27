#ifndef MATRIXSERVER_SERVER_H
#define MATRIXSERVER_SERVER_H

#include <sys/types.h>
#include <vector>
#include <memory>
#include <mutex>
#include <thread>

#include <Screen.h>
#include <App.h>
#include <IBidirectionalRenderer.h>
#include <IRenderer.h>
#include <matrixserver.pb.h>
#include <TcpServer.h>
#include <UnixSocketServer.h>
#include <IpcServer.h>
#include <Joystick.h>
#include <MessageDispatcher.h>

#include "DefaultAppLauncher.h"
#include "RendererRegistry.h"

class Server {
public:

    ~Server();

    Server(std::shared_ptr<IRenderer>, matrixserver::ServerConfig &);

    bool tick();

    void handleRequest(std::shared_ptr<UniversalConnection> connection,
                       std::shared_ptr<matrixserver::MatrixServerMessage> message);

    void newConnectionCallback(std::shared_ptr<UniversalConnection>);

    void addRenderer(std::shared_ptr<IRenderer>);

    std::shared_ptr<App> getAppByID(int searchID);

    void stopDefaultApp();

private:
    // Dispatcher setup — called once from the constructor.
    void setupDispatcher();

    // Per-message-type handler methods registered with dispatcher_.
    void handleRegisterApp(std::shared_ptr<UniversalConnection> conn,
                           std::shared_ptr<matrixserver::MatrixServerMessage> msg);
    void handleGetServerInfo(std::shared_ptr<UniversalConnection> conn,
                             std::shared_ptr<matrixserver::MatrixServerMessage> msg);
    void handleRequestScreenAccess(std::shared_ptr<UniversalConnection> conn,
                                   std::shared_ptr<matrixserver::MatrixServerMessage> msg);
    void handleSetScreenFrame(std::shared_ptr<UniversalConnection> conn,
                              std::shared_ptr<matrixserver::MatrixServerMessage> msg);
    void handleAppLifecycle(std::shared_ptr<UniversalConnection> conn,
                            std::shared_ptr<matrixserver::MatrixServerMessage> msg);
    void handleAppParamSchema(std::shared_ptr<UniversalConnection> conn,
                              std::shared_ptr<matrixserver::MatrixServerMessage> msg);
    void handleAppParamValues(std::shared_ptr<UniversalConnection> conn,
                              std::shared_ptr<matrixserver::MatrixServerMessage> msg);

    std::mutex appsMutex;
    std::vector<std::shared_ptr<App>> apps;
    RendererRegistry registry_;
    boost::asio::io_context ioContext;
    std::unique_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> ioWork;
    matrixserver::ServerConfig & serverConfig;
    TcpServer tcpServer;
//    UnixSocketServer unixServer;
    IpcServer ipcServer;
    std::unique_ptr<std::thread> ioThread;
    std::vector<std::shared_ptr<UniversalConnection>> connections;
    JoystickManager joystickmngr;
    MessageDispatcher dispatcher_;
    DefaultAppLauncher defaultAppLauncher_;
};


#endif //MATRIXSERVER_SERVER_H
