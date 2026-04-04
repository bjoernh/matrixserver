# AGENTS.md - Coding Agent Guidelines for matrixserver

## Project Overview

C++17 LED matrix screen server for the LEDCube project. Modular architecture with
abstract renderer backends (Simulator, FPGA FTDI/SPI, RGB Matrix) and protobuf-based
client-server communication over TCP/Unix sockets/IPC.

## Build System (CMake)

### Dependencies

```
libeigen3-dev cmake libboost-all-dev libasound2-dev libprotobuf-dev protobuf-compiler libimlib2-dev
```

On Raspberry Pi additionally: `wiringpi`

### Build Commands

```bash
# Simulator only (default, non-Raspberry Pi)
mkdir -p build && cd build && cmake .. && make

# Debug build
mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Debug .. && make

# Hardware build — select one backend:
mkdir -p build && cd build && cmake -DHARDWARE_BACKEND=FPGA_FTDI .. && make
mkdir -p build && cd build && cmake -DHARDWARE_BACKEND=FPGA_RPISPI .. && make
mkdir -p build && cd build && cmake -DHARDWARE_BACKEND=RGB_MATRIX .. && make

# Rebuild after changes (from build/)
make

# Build specific targets
make matrix_server_simulator
make matrix_server          # requires HARDWARE_BACKEND to be set
make testAll
make common
make server
make matrixapplication
```

### Build Targets

| Target | Binary | Description |
|--------|--------|-------------|
| `matrix_server_simulator` | `server_simulator/matrix_server_simulator` | Always built; uses WebSocketSimulatorRenderer |
| `matrix_server` | `server_hardware/matrix_server` | Built when `HARDWARE_BACKEND` is set |

### HARDWARE_BACKEND Values

| Value | Renderer | Interface |
|-------|----------|-----------|
| `FPGA_FTDI` | `FPGARendererFTDI` | FTDI USB to IceBreaker FPGA |
| `FPGA_RPISPI` | `FPGARendererRPISPI` | Raspberry Pi SPI to FPGA |
| `RGB_MATRIX` | `RGBMatrixRenderer` | Raspberry Pi GPIO to HUB75 panels |

### Tests (Catch v1, single-header)

**IMPORTANT:** The `tests/` subdirectory is NOT added in the root CMakeLists.txt. You must
add `add_subdirectory(tests)` to the root `CMakeLists.txt` or build from the tests directory
to compile tests.

```bash
# Build and run all tests (from build/)
make testAll && ./tests/testAll

# Run tests matching a name pattern
./tests/testAll "cobs string encode/decode"

# Run tests by tag
./tests/testAll [cobs]
./tests/testAll [screen]

# List all available tests
./tests/testAll --list-tests

# Verbose output
./tests/testAll -s
```

Test files use Catch v1 macros: `TEST_CASE`, `SECTION`, `REQUIRE` (fatal),
`CHECK` (non-fatal), `INFO`, `WARN`. Tag tests with `[module]` convention.

### Docker

```bash
docker build -t matrixserver .                              # amd64 simulator
docker build -f Dockerfile.arm64 --build-arg HARDWARE_BACKEND=FPGA_RPISPI -t matrixserver-rpi .
```

### CI

- GitHub Actions (`.github/workflows/docker-build-push.yml`): Builds simulator (AMD64) on tag push
- GitHub Actions (`.github/workflows/docker-build-push-rpi.yml`): Builds hardware target (ARM64) with `HARDWARE_BACKEND=FPGA_RPISPI` on tag push
- GitHub Actions (`.github/workflows/docker-build-push-simulator-arm64.yml`): Builds simulator (ARM64) on tag push

## Code Style Guidelines

### Language Standard

C++17.

### File Organization

- One class per file pair: `ClassName.h` + `ClassName.cpp` (PascalCase filenames)
- Directories are modules, each with its own `CMakeLists.txt`
- Module structure: `common/` (shared), `server/`, `application/`, `renderer/`,
  `server_simulator/`, `server_hardware/` (executables), `tests/`

### Header Guards

Use traditional `#ifndef` / `#define` / `#endif` guards (NOT `#pragma once`):

```cpp
#ifndef MATRIXSERVER_CLASSNAME_H
#define MATRIXSERVER_CLASSNAME_H
// ...
#endif //MATRIXSERVER_CLASSNAME_H
```

### Include Order

1. Standard library headers (`<vector>`, `<iostream>`, `<memory>`)
2. Third-party headers (`<boost/log/trivial.hpp>`, `<Eigen/Dense>`, `<google/protobuf/...>`)
3. Project headers (quotes for same-directory: `"Server.h"`, angle brackets cross-module: `<Screen.h>`)

No blank line separators between groups (informal grouping).

### Naming Conventions

| Element              | Convention          | Examples                                        |
|----------------------|---------------------|-------------------------------------------------|
| Classes              | PascalCase          | `Server`, `CubeApplication`, `SocketConnection` |
| Abstract/Interfaces  | `I` prefix          | `IRenderer`                                     |
| Methods/Functions    | camelCase           | `getAppId()`, `setPixel3D()`, `handleRequest()` |
| Member variables     | camelCase (no prefix)| `appId`, `screenData`, `serverConfig`           |
| Local variables      | camelCase           | `tempCon`, `returnVal`, `sendBuffer`            |
| Constructor params   | `set` prefix        | `setWidth`, `setHeight`, `setRenderer`          |
| Macros/Constants     | SCREAMING_SNAKE     | `DEFAULTFPS`, `MAXFPS`, `CUBESIZE`              |
| Enum types           | PascalCase          | `AppState`, `Rotation`, `ScreenNumber`          |
| Enum values          | camelCase           | `running`, `paused`, `rot0`, `anyScreen`        |
| Files                | PascalCase          | `Server.h`, `CubeApplication.cpp`               |
| Typedefs             | camelCase + Type    | `colorPixelArrayType`                           |

### Formatting

- **Indentation:** 4 spaces (no tabs)
- **Braces:** K&R style (opening brace on same line)
- **Line length:** No strict limit; lines commonly reach 120-150 chars
- **Spaces:** After keywords (`if (`, `for (`), around binary operators, no space before function parens

```cpp
class Server {
public:
    void handleRequest(std::shared_ptr<UniversalConnection> con,
                       std::shared_ptr<matrixserver::MatrixServerMessage> message) {
        if (message->appid() == 0) {
            // ...
        }
    }
};
```

### Constructor Initializer Lists

Colon on same line, members indented 8 spaces, one per line:

```cpp
Server::Server(std::shared_ptr<IRenderer> setRenderer, matrixserver::ServerConfig &setServerConfig) :
        ioContext(),
        ioWork(new boost::asio::io_service::work(ioContext)),
        serverConfig(setServerConfig),
        renderer(setRenderer) {
```

### Types and Smart Pointers

- Prefer `std::shared_ptr<>` for shared ownership (dominant pattern in codebase)
- Use `std::unique_ptr<>` for exclusive ownership
- Use `auto` with `std::make_shared` and iterators
- Avoid raw `new` -- existing raw `new` for `boost::thread*` is a known issue
- Avoid C-style casts; use `static_cast<>`, `reinterpret_cast<>`, etc.

### Error Handling

- **Logging:** Use Boost.Log with component prefix: `BOOST_LOG_TRIVIAL(level) << "[Component] message"`
- Log levels: `trace`, `debug`, `info`, `warning`, `error`, `fatal`
- **Exceptions:** Catch by `const&`, not by value
- **Connection state:** Use `isDead()` / `setDead()` pattern for broken connections
- **Return values:** Use `bool` for simple success/failure; protobuf `Status` enum for protocol errors
- No custom exception classes in the codebase

```cpp
try {
    // ...
} catch (const boost::system::system_error &e) {
    BOOST_LOG_TRIVIAL(error) << "[Component] " << e.what();
}
```

### Protobuf

- Proto3 syntax, package `matrixserver`
- Definition: `common/protobuf/matrixserver.proto`
- Generated code is built automatically by CMake's `protobuf_generate_cpp`
- `ServerConfig` includes `tickIntervalMs` (tick loop sleep in milliseconds)
- `ScreenInfo` includes `screenRotation`, `offsetX`, `offsetY` for hardware-specific screen mapping

### ServerSetup

`server/ServerSetup.h` provides the `ServerSetup` namespace with:
- `HardwareType` enum: `Simulator`, `FPGA_FTDI`, `FPGA_RPISPI`, `RGB_MATRIX`
- `handleServerConfig(argc, argv, serverConfig)` — parses CLI args, loads/generates JSON config
- `createDefaultCubeConfig(serverConfig, hwType)` — generates hardware-specific defaults including screen orientations
- `createScreensFromConfig(serverConfig)` — creates `Screen` objects from config (reads offsetX/Y/rotation from JSON)

### Key Design Patterns

- **Strategy:** `IRenderer` interface with multiple backends
- **Template Method:** `MatrixApplication::internalLoop()` calls virtual `loop()`
- **Observer/Callback:** `std::function` callbacks for connection events
- **Bridge:** `UniversalConnection` abstracts TCP/IPC transports
- **enable_shared_from_this:** Used by `SocketConnection`, `IpcConnection`
- **Compile-time renderer selection:** `server_hardware/main.cpp` uses `#ifdef BACKEND_*` defines set by CMake to select the renderer type at compile time

### Writing Tests

Place test files in `tests/`. Follow the Catch v1 pattern:

```cpp
#include "catch.hpp"
#include <ModuleUnderTest.h>

TEST_CASE("descriptive name", "[module-tag]") {
    // setup
    SECTION("sub-case description") {
        REQUIRE(expected == actual);    // fatal
        CHECK(expected == actual);      // non-fatal
    }
}
```

### Things to Avoid

- Do NOT use `#pragma once` (project uses `#ifndef` guards)
- Do NOT use C-style casts (use C++ casts; existing C-style casts are flagged with TODOs)
- Do NOT catch exceptions by value (use `const&`)
- Do NOT use `NULL` (use `nullptr`)
- Do NOT add `using namespace std;` in headers
- Do NOT use raw `new` without wrapping in a smart pointer
- Do NOT add new server_* executable directories — use `server_simulator/` or `server_hardware/`
- Do NOT hard-code screen orientations/offsets in main.cpp — put them in `createDefaultCubeConfig()`
