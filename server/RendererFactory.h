#ifndef MATRIXSERVER_RENDERERFACTORY_H
#define MATRIXSERVER_RENDERERFACTORY_H

#include <IRenderer.h>
#include <Screen.h>
#include <ServerSetup.h>
#include <functional>
#include <memory>
#include <string_view>
#include <vector>

using HardwareRendererFactory = std::function<std::shared_ptr<IRenderer>(
    const std::vector<std::shared_ptr<Screen>> &)>;

HardwareRendererFactory makeHardwareRendererFactory(std::string_view backend);
ServerSetup::HardwareType hardwareTypeForBackend(std::string_view backend);
std::vector<std::string> availableBackends();

#endif // MATRIXSERVER_RENDERERFACTORY_H
