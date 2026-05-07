#ifndef MATRIXSERVER_SERVERBOOTSTRAP_H
#define MATRIXSERVER_SERVERBOOTSTRAP_H

#include <IRenderer.h>
#include <Screen.h>
#include <ServerSetup.h>
#include <functional>
#include <memory>
#include <vector>

namespace ServerBootstrap {

/**
 * Factory function type for creating a hardware renderer.
 * Receives the list of configured screens and returns a renderer.
 * If the factory is null (or returns nullptr), the server runs in
 * simulator-only mode: a WebSocketSimulatorRenderer is the sole renderer
 * with pixel streaming enabled.
 * If the factory returns a non-null renderer, that renderer becomes the
 * primary; a WebSocketSimulatorRenderer is added as a secondary with
 * streamPixels=false (parameter-only channel).
 */
using HardwareRendererFactory = std::function<std::shared_ptr<IRenderer>(const std::vector<std::shared_ptr<Screen>> &)>;

/**
 * Full server bootstrap: parse config, create screens and renderers,
 * run the tick loop, and return an exit code.
 *
 * @param argc          Command-line argument count (forwarded to ServerSetup).
 * @param argv          Command-line arguments (forwarded to ServerSetup).
 * @param hwType        Hardware type used when generating a default config.
 *                      Pass HardwareType::Simulator for the simulator binary.
 * @param makeHardwareRenderer
 *                      Callable that constructs the primary hardware renderer,
 *                      or nullptr for simulator-only mode.
 * @return              0 on clean shutdown, 1 on fatal error.
 */
int runServer(int argc, char **argv, ServerSetup::HardwareType hwType, HardwareRendererFactory makeHardwareRenderer);

} // namespace ServerBootstrap

#endif // MATRIXSERVER_SERVERBOOTSTRAP_H
