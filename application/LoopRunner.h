#ifndef MATRIXSERVER_LOOPRUNNER_H
#define MATRIXSERVER_LOOPRUNNER_H

#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <functional>

// ---------------------------------------------------------------------------
// AppState — application lifecycle states, used by the LoopRunner state machine
// and exposed via MatrixApplication::getAppState().
// ---------------------------------------------------------------------------

enum class AppState { starting, running, paused, ended, killed, failure };

// ---------------------------------------------------------------------------
// LoopRunner
//
// Owns the main application thread, FPS regulation, the screen-access
// condition-variable handshake, load tracking, and the exception-safety net
// around each iteration.  The user's loop() plugs in via the body callback;
// an optional post-body callback handles per-iteration housekeeping
// (e.g. connection health-check).
// ---------------------------------------------------------------------------

class LoopRunner {
  public:
    using BodyCallback = std::function<bool()>;
    using PostBodyCallback = std::function<void()>;

    void start(BodyCallback body, PostBodyCallback postBody = nullptr);
    void stop();

    void setState(AppState state);
    AppState getState() const;

    void setFps(int fps);
    int getFps() const;

    float getLoad() const;

    // Called by the message-dispatcher handler to signal that the server has
    // acknowledged our frame, releasing the condvar wait in runLoop().
    void signalScreenAccess();

  private:
    void runLoop(BodyCallback body, PostBodyCallback postBody);
    long micros();

    std::unique_ptr<std::thread> thread_;
    std::atomic<bool> running_{false};

    std::mutex screenAccessMutex_;
    std::condition_variable screenAccessCv_;
    bool screenAccessGranted_ = false;

    int fps_ = 40;
    float load_ = 0.0f;
    AppState state_ = AppState::starting;
};

#endif // MATRIXSERVER_LOOPRUNNER_H
