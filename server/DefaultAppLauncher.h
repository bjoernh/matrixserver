#ifndef MATRIXSERVER_DEFAULTAPPLAUNCHER_H
#define MATRIXSERVER_DEFAULTAPPLAUNCHER_H

#include <string>
#include <sys/types.h>

/// Manages the lifecycle of the "default app" — the binary launched when no
/// client app is connected to the server.
///
/// The binary path is resolved once at construction time from the
/// MATRIXSERVER_DEFAULT_APP environment variable (first whitespace-separated
/// token). If the variable is absent, /usr/local/bin/MainMenu is used.
///
/// Usage pattern (mirrors original Server behaviour):
///   - Call launchIfNotRunning() when the apps list is empty.
///   - Call markAppsPresent() when at least one app registers, so the next
///     vacancy triggers a fresh launch.
///   - Call stop() on server shutdown to SIGTERM → SIGKILL the child.
class DefaultAppLauncher {
  public:
    DefaultAppLauncher();

    /// Launch the default app (fork/exec) if it has not been launched since
    /// the last markAppsPresent() call.
    void launchIfNotRunning();

    /// Signal that at least one app is now connected. Resets the launched-once
    /// flag so a future launchIfNotRunning() will start a fresh instance, and
    /// reaps the child if it already exited on its own.
    void markAppsPresent();

    /// Returns true if the child process is currently running.
    bool isRunning() const { return childPid_ > 0; }

    /// Send SIGTERM and wait up to 3 s; SIGKILL if still alive.
    void stop();

    /// The resolved binary path (for logging).
    const std::string& binaryPath() const { return binaryPath_; }

  private:
    std::string binaryPath_;
    pid_t childPid_ = -1;
    bool launched_ = false;
};

#endif // MATRIXSERVER_DEFAULTAPPLAUNCHER_H
