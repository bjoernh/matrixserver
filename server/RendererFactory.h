#ifndef MATRIXSERVER_RENDERERFACTORY_H
#define MATRIXSERVER_RENDERERFACTORY_H

#include <ServerBootstrap.h>
#include <ServerSetup.h>
#include <string>
#include <string_view>
#include <vector>

/**
 * Returns a HardwareRendererFactory for the given backend string.
 * Returns nullptr for "simulator" (simulator-only mode).
 * Throws std::invalid_argument for unknown or not-compiled-in backends.
 *
 * Accepted backend strings: simulator, fpga-ftdi, fpga-rpispi, rgb-matrix
 */
ServerBootstrap::HardwareRendererFactory makeHardwareRendererFactory(std::string_view backend);

/**
 * Maps a backend string to the corresponding HardwareType enum value.
 * Throws std::invalid_argument for unknown backend strings.
 */
ServerSetup::HardwareType hardwareTypeForBackend(std::string_view backend);

/**
 * Returns the list of backends compiled into this binary.
 * Always includes "simulator"; adds hardware backends conditionally.
 */
std::vector<std::string> availableBackends();

#endif // MATRIXSERVER_RENDERERFACTORY_H
