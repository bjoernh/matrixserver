#ifndef MATRIXSERVER_MATRIXAPPLICATION_H
#define MATRIXSERVER_MATRIXAPPLICATION_H

#include <thread>

#include "LoopRunner.h"
#include <IpcConnection.h>
#include <Screen.h>
#include <AnimationParams.h>
#include <TcpClient.h>
#include <UnixSocketClient.h>
#include <matrixserver.pb.h>
#include <mutex>

#include "MessageDispatcher.h"
#include "InputState.h"

inline constexpr int DEFAULTFPS = 40;
inline constexpr int MAXFPS = 200;
inline constexpr int MINFPS = 1;

inline constexpr const char* DEFAULTSERVERURI = "tcp://127.0.0.1:2017";

// Legacy constants kept for backward compatibility
inline constexpr const char* DEFAULTSERVERADRESS = "127.0.0.1";
inline constexpr const char* DEFAULTSERVERPORT = "2017";

class MatrixApplication {
public:
  MatrixApplication(int fps = DEFAULTFPS,
                    std::string serverUri = DEFAULTSERVERURI,
                    std::string appName = "MatrixApp");

  ~MatrixApplication();

  void renderToScreens();

  int getFps();

  void setFps(int fps);

  AppState getAppState();

  float getLoad();

  void start();

  bool pause();

  bool resume();

  void stop();

  int getBrightness();

  void setBrightness(int setBrightness);

  virtual bool loop() = 0;

protected:
  std::vector<std::shared_ptr<Screen>> screens;

  long micros();

  matrixserver::AnimationParams params;
  std::string appName;

private:
  bool connect(const std::string &server_uri);

  void checkConnection();

  void registerAtServer();

  void handleRequest(std::shared_ptr<UniversalConnection>,
                     std::shared_ptr<matrixserver::MatrixServerMessage>);

  // Dispatcher setup — called once from the constructor.
  void setupDispatcher();

  // Per-message-type handler methods registered with dispatcher_.
  void handleRegisterApp(std::shared_ptr<UniversalConnection> conn,
                         std::shared_ptr<matrixserver::MatrixServerMessage> msg);
  void handleGetServerInfo(std::shared_ptr<UniversalConnection> conn,
                           std::shared_ptr<matrixserver::MatrixServerMessage> msg);
  void handleAppPause(std::shared_ptr<UniversalConnection> conn,
                      std::shared_ptr<matrixserver::MatrixServerMessage> msg);
  void handleAppAlive(std::shared_ptr<UniversalConnection> conn,
                      std::shared_ptr<matrixserver::MatrixServerMessage> msg);
  void handleAppResume(std::shared_ptr<UniversalConnection> conn,
                       std::shared_ptr<matrixserver::MatrixServerMessage> msg);
  void handleAppKill(std::shared_ptr<UniversalConnection> conn,
                     std::shared_ptr<matrixserver::MatrixServerMessage> msg);
  void handleScreenAccess(std::shared_ptr<UniversalConnection> conn,
                          std::shared_ptr<matrixserver::MatrixServerMessage> msg);
  void handleJoystickData(std::shared_ptr<UniversalConnection> conn,
                          std::shared_ptr<matrixserver::MatrixServerMessage> msg);
  void handleImuData(std::shared_ptr<UniversalConnection> conn,
                     std::shared_ptr<matrixserver::MatrixServerMessage> msg);
  void handleAudioData(std::shared_ptr<UniversalConnection> conn,
                       std::shared_ptr<matrixserver::MatrixServerMessage> msg);
  void handleSetAppParam(std::shared_ptr<UniversalConnection> conn,
                         std::shared_ptr<matrixserver::MatrixServerMessage> msg);
  void handleGetAppParams(std::shared_ptr<UniversalConnection> conn,
                          std::shared_ptr<matrixserver::MatrixServerMessage> msg);

  MessageDispatcher dispatcher_;
  InputState inputState_;

  int appId;

  int brightness;
  std::string serverUri;
  std::shared_ptr<UniversalConnection> connection;
  std::unique_ptr<std::thread> ioThread;
  boost::asio::io_context io_context;
  matrixserver::ServerConfig serverConfig;

  LoopRunner runner_;

public:
  static float latestSimulatorImuX;
  static float latestSimulatorImuY;
  static float latestSimulatorImuZ;
  static float latestSimulatorGyroX;
  static float latestSimulatorGyroY;
  static float latestSimulatorGyroZ;
  static std::mutex simulatorImuMutex;

  static uint8_t latestAudioVolume;
  static std::vector<uint8_t> latestAudioFrequencies;
  static std::mutex audioDataMutex;
};

#endif // MATRIXSERVER_MATRIXAPPLICATION_H