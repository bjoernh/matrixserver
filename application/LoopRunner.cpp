#include "LoopRunner.h"

#include <boost/log/trivial.hpp>
#include <sys/time.h>

// ---------------------------------------------------------------------------
// start / runLoop
// ---------------------------------------------------------------------------

void LoopRunner::start(BodyCallback body, PostBodyCallback postBody) {
    thread_ = std::make_unique<std::thread>(&LoopRunner::runLoop, this, std::move(body), std::move(postBody));
}

void LoopRunner::runLoop(BodyCallback body, PostBodyCallback postBody) {
    running_.store(true);
    bool keepGoing = true;
    while (keepGoing) {
        try {
            auto startTime = micros();
            if (state_ == AppState::running) {
                std::unique_lock<std::mutex> lock(screenAccessMutex_);
                screenAccessGranted_ = false;
                keepGoing = body();
                screenAccessCv_.wait(lock, [this] { return screenAccessGranted_ || !running_.load(); });
                screenAccessGranted_ = false;
            }
            if (state_ == AppState::killed) {
                keepGoing = false;
            }
            if (postBody) {
                postBody();
            }
            auto sleepTime = (1000000 / fps_) - (micros() - startTime);
            if (sleepTime > 0) {
                usleep(sleepTime);
            }
            load_ = 1.0f - ((float)sleepTime / (1000000.0f / (float)fps_));
        } catch (const std::exception &e) {
            BOOST_LOG_TRIVIAL(error) << "[LoopRunner] Loop error: " << e.what();
        }
    }
    running_.store(false);
}

// ---------------------------------------------------------------------------
// stop — signal shutdown, wake condvar, join thread
// ---------------------------------------------------------------------------

void LoopRunner::stop() {
    setState(AppState::killed);
    {
        std::lock_guard<std::mutex> lock(screenAccessMutex_);
        running_.store(false);
        screenAccessCv_.notify_all();
    }
    if (thread_ && thread_->joinable()) {
        thread_->join();
    }
    thread_.reset();
}

// ---------------------------------------------------------------------------
// State / FPS / load accessors
// ---------------------------------------------------------------------------

void LoopRunner::setState(AppState state) { state_ = state; }

AppState LoopRunner::getState() const { return state_; }

void LoopRunner::setFps(int fps) { fps_ = fps; }

int LoopRunner::getFps() const { return fps_; }

float LoopRunner::getLoad() const { return load_; }

// ---------------------------------------------------------------------------
// Screen-access handshake
// ---------------------------------------------------------------------------

void LoopRunner::signalScreenAccess() {
    std::lock_guard<std::mutex> lock(screenAccessMutex_);
    screenAccessGranted_ = true;
    screenAccessCv_.notify_one();
}

// ---------------------------------------------------------------------------
// Time helper (mirrors MatrixApplication::micros())
// ---------------------------------------------------------------------------

long LoopRunner::micros() {
    struct timeval tp;
    gettimeofday(&tp, nullptr);
    return tp.tv_sec * 1000000 + tp.tv_usec;
}
