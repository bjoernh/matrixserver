#include "DefaultAppLauncher.h"

#include <boost/log/trivial.hpp>
#include <cstdlib>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::string resolveBinaryPath() {
    const char *envVal = std::getenv("MATRIXSERVER_DEFAULT_APP");
    std::string cmd = envVal ? std::string(envVal) : std::string("/usr/local/bin/MainMenu");
    // Trim to first token in case the env var still contains shell arguments.
    auto pos = cmd.find_first_of(" \t");
    return (pos != std::string::npos) ? cmd.substr(0, pos) : cmd;
}

// ---------------------------------------------------------------------------
// DefaultAppLauncher
// ---------------------------------------------------------------------------

DefaultAppLauncher::DefaultAppLauncher() : binaryPath_(resolveBinaryPath()) {}

void DefaultAppLauncher::launchIfNotRunning() {
    if (launched_) {
        return;
    }
    BOOST_LOG_TRIVIAL(info) << "[Server] Starting default app: " << binaryPath_;
    pid_t pid = fork();
    if (pid == 0) {
        // Child: redirect stdout/stderr to /dev/null and exec the app.
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull >= 0) {
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }
        setsid(); // detach from server's process group
        execl(binaryPath_.c_str(), binaryPath_.c_str(), nullptr);
        _exit(1); // execl failed
    } else if (pid > 0) {
        childPid_ = pid;
        BOOST_LOG_TRIVIAL(debug) << "[Server] Default app PID: " << pid;
    } else {
        BOOST_LOG_TRIVIAL(warning) << "[Server] fork() failed for default app";
    }
    launched_ = true;
}

void DefaultAppLauncher::markAppsPresent() {
    launched_ = false;
    // Reap the child if it already exited on its own (e.g. another app took over).
    if (childPid_ > 0 && waitpid(childPid_, nullptr, WNOHANG) != 0) {
        childPid_ = -1;
    }
}

void DefaultAppLauncher::stop() {
    if (childPid_ > 0) {
        BOOST_LOG_TRIVIAL(info) << "[Server] Stopping default app (PID " << childPid_ << ")";
        kill(childPid_, SIGTERM);
        // Wait up to 3 s for graceful exit, then force-kill.
        for (int i = 0; i < 30; ++i) {
            usleep(100'000);
            if (waitpid(childPid_, nullptr, WNOHANG) != 0) {
                childPid_ = -1;
                return;
            }
        }
        BOOST_LOG_TRIVIAL(warning) << "[Server] Default app did not exit, sending SIGKILL";
        kill(childPid_, SIGKILL);
        waitpid(childPid_, nullptr, 0);
        childPid_ = -1;
    }
}
