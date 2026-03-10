[![Build Matrixserver Simulator (AMD64)](https://github.com/bjoernh/matrixserver/actions/workflows/docker-build-push.yml/badge.svg)](https://github.com/bjoernh/matrixserver/actions/workflows/docker-build-push.yml)
[![Build Matrixserver RPi (ARM64)](https://github.com/bjoernh/matrixserver/actions/workflows/docker-build-push-rpi.yml/badge.svg)](https://github.com/bjoernh/matrixserver/actions/workflows/docker-build-push-rpi.yml)

# LEDCube matrixserver

This is a screenserver for the purpose of being used with differently orientated LED Matrix panels. 
It currently has been implemented for the LEDCube project, but it can also be used in simple, 
planar screen orientations, as well as other complex screen orientations.

# Dependencies

on raspbian and ubuntu:
`sudo apt install git libeigen3-dev cmake wiringpi libboost-all-dev libasound2-dev libprotobuf-dev protobuf-compiler libimlib2-dev`

# Building

**make sure you have cloned with submodules** `git clone --recursive`  
tested on Ubuntu, Raspbian & macOS.

By default, on macOS and standard Ubuntu setups, only the development targets (like `server_simulator`) are built to avoid missing hardware dependencies.

To build the project for a standard development environment:
`mkdir build && cd build && cmake .. && make`

To build the project for Raspberry Pi (including hardware-specific variants like `server_FPGA_FTDI`, `server_RGBMatrix`, etc.):
`mkdir build && cd build && cmake -DBUILD_RASPBERRYPI=ON .. && make`

To build and install the project to a local directory (e.g., `./install`):
```bash
mkdir -p build && cd build
cmake -DCMAKE_INSTALL_PREFIX=$(pwd)/../install ..
make -j$(nproc)
make install
```

# Releases and Docker

Pre-built binaries and Docker images are automatically generated for every repository tag.

## Debian Packages
Pre-compiled `.deb` packages for both `amd64` (Simulator targets) and `arm64` (Raspberry Pi hardware targets) are available on the [GitHub Releases](https://github.com/bjoernh/matrixserver/releases) page. You can easily install them on compatible systems using:
`sudo dpkg -i matrixserver-*.deb`

## Docker Images
To run the server without compiling or installing dependencies, you can use the pre-built Docker images hosted on the GitHub Container Registry (GHCR). 

**Simulator (AMD64)**
```bash
docker pull ghcr.io/bjoernh/matrixserver-simulator:latest
# Expose the Matrix Server port (2017) and Simulator WebSocket port (1337)
docker run -it --rm -p 2017:2017 -p 1337:1337 ghcr.io/bjoernh/matrixserver-simulator:latest server_simulator
```
To test the matrix server without a physical cube, you can use the CUBE Simulator:
Open the CUBE Simulator in your browser:
**https://bjoernh.github.io/CubeSimulator/**

Use the default address `ws://localhost:1337` to connect the CUBE Simulator to the matrix server.

![Picture of Cube Simulator](LEDCubeSim.jpg)

**Raspberry Pi (ARM64)**
```bash
docker pull ghcr.io/bjoernh/matrixserver-rpi:latest
# Hardware access usually requires privileges or mapping specific /dev devices
docker run -it --rm --privileged -v /dev:/dev ghcr.io/bjoernh/matrixserver-rpi:latest server_RGBMatrix
```
*(Note: You can pass any of the standard server parameters, such as `--config`, at the end of the `docker run` command).*

# Server Configuration

All `server_*` targets share a unified command-line interface for configuration and startup:

*   **`-h, --help`**: Display available command-line options.
*   **`--config <path>`**: Path to the `matrixServerConfig.json` configuration file. If not provided, the server checks the current directory or prompts you to generate a default one.
*   **`--address <ip>`**: Override the server address specified in the configuration file.
*   **`--use-deprecated-tcp-connection`**: (Simulator only) Use the legacy TCP connection for the simulator renderer.

When starting a server without an existing configuration file, it will explicitly prompt you `[y/N]` before creating a default `matrixServerConfig.json` in the current directory. If you decline, it runs with a default in-memory configuration.

# Important: Running an Application

The matrix server only acts as a display driver. By itself, it will only maintain the connection and show a blank screen or default background. 

To actually see something rendered on your led matrix or the simulator, **you must start a client application** after the server is running. The client application connects to the matrix server and sends the actual pixel data to be displayed. 

You can find example applications to run in the `exampleApplications` repository (e.g., `cubetestapp` or `PixelFlow3`).

# Example: Running the server on a Raspberry Pi with an IceBreaker board

If you have an IceBreaker board with HUB75 PMOD:  
* at first load the FPGA with the `rgb_panel` project example (https://github.com/squarewavedot/ice40-playground/tree/master/projects/rgb_panel)   
* hook up the icebreaker to the Raspberry Pi via USB
* compile and start the `server_FPGA_FTDI` target (in the `build` folder `make server_FPGA_FTDI`)
* In another Terminal compile and start the `cubetestapp` or `PixelFlow` or any other target from the exampleApplications repository.


## Repository Structure & Modules

The project is thoughtfully divided into logical directories that separate the server daemon, the display rendering technologies, the shared communication protocols, and client application libraries:

*   **`common`**
    *   Defines the core `matrixserver` Protobuf messages used for exchanging pixel data and configurations between clients and the server.
    *   Contains the underlying connection implementations (IPC using Boost Message Queues for local performance, Unix Sockets, TCP Sockets for network streams).
    *   Provides foundational classes like `Screen`, `Color`, and `Cobs` encoding.

*   **`renderer`**
    *   Contains interchangeable rendering backends that the server uses to output the final pixel buffers to physical or virtual displays.
    *   *Supported Renderers include:*
        *   **`RGBMatrixRenderer`**: Hardware interface driving HUB75 panels directly from Raspberry Pi GPIOs (via `rpi-rgb-led-matrix`).
        *   **`WebSocketSimulatorRenderer`**: Network interface streaming pixels to the web-based `CubeSimulator` project.
        *   **`FPGAFTDIRenderer` & `FPGASPIRenderer`**: Protocol implementations for sending pixel data to an IceBreaker FPGA board acting as the HUB75 driver, via USB FTDI or RPi SPI.
        *   **`TestRenderer`**: A simple 2D previewer that uses OpenCV windows, entirely software-based.

*   **`server`**
    *   The core daemon logic containing the `Server` class that accepts connections, validates configuration parameters, and routes incoming application frames to the active renderers.
    *   Includes a unified `ServerSetup` utility to handle configuration parsing consistently.

*   **`application`**
    *   A client-side C++ library containing base classes like `MatrixApplication` and `CubeApplication`. 
    *   These provide a high-level API with convenient drawing methods (e.g., `setPixel3D`, coordinate mapping) for writing custom programs that connect to the screen server.

*   **`server_*` directories (The Executables)**
    *   These directories contain the `main.cpp` programs that instantiate the server with a specific set of active renderers targeted for a given platform:
        *   **`server_simulator`**: The default build target. Combines the server with the `WebSocketSimulatorRenderer` to interact with the web simulator.
        *   **`server_RGBMatrix`**: The production target for Raspberry Pi direct GPIO matrix driving.
        *   **`server_FPGA_FTDI` / `server_FPGA_RPISPI`**: Targets for offloading the Hub75 driving to an external IceBreaker FPGA.
        *   **`server_testapp`**: Builds if OpenCV is installed. Opens simple 2D windows on the host machine representing each matrix plane, useful for debugging visual logic without the 3D emulator overhead.

*   **`MainMenu`**
    *   A built-in example client application that provides a launch interface for the cube.
