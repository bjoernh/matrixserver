#ifndef MATRIXSERVER_MATRIXAPPLICATION_H
#define MATRIXSERVER_MATRIXAPPLICATION_H

#include <thread>
#include <atomic>
#include <condition_variable>

#include <IpcConnection.h>
#include <Screen.h>
#include <AnimationParams.h>
#include <TcpClient.h>
#include <UnixSocketClient.h>
#include <matrixserver.pb.h>
#include <mutex>

#define DEFAULTFPS 40
#define MAXFPS 200
#define MINFPS 1

#define DEFAULTSERVERURI "tcp://127.0.0.1:2017"

// Legacy defines kept for backward compatibility
#define DEFAULTSERVERADRESS "127.0.0.1"
#define DEFAULTSERVERPORT "2017"

enum class AppState { starting, running, paused, ended, killed, failure };

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
  void internalLoop();

  bool connect(const std::string &server_uri);

  void checkConnection();

  void registerAtServer();

  void handleRequest(std::shared_ptr<UniversalConnection>,
                     std::shared_ptr<matrixserver::MatrixServerMessage>);

  int appId;
  int fps;
  float load;

  int brightness;
  std::string serverUri;
  std::shared_ptr<UniversalConnection> connection;
  std::unique_ptr<std::thread> mainThread;
  std::unique_ptr<std::thread> ioThread;
  AppState appState;
  boost::asio::io_context io_context;
  matrixserver::ServerConfig serverConfig;

  // Condition variable for the render-sync handshake (replaces cross-thread
  // mutex lock/unlock which was not exception-safe).
  std::mutex screenAccessMutex_;
  std::condition_variable screenAccessCv_;
  bool screenAccessGranted_ = false;
  std::atomic<bool> loopRunning_{false};

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