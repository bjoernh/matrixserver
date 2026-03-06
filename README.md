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

# Quickstart

If you have an IceBreaker board with HUB75 PMOD:  
* at first load the FPGA with the `rgb_panel` project example (https://github.com/squarewavedot/ice40-playground/tree/master/projects/rgb_panel)   
* hook up the icebreaker to the Raspberry Pi via USB
* compile and start the `server_FPGA_FTDI` target (in the `build` folder `make server_FPGA_FTDI`)
* In another Terminal compile and start the `cubetestapp` or `PixelFlow` or any other target from the exampleApplications repository.


## The Project is divided into multiple modules:

* common

	* protobuf message definition for communication
        * multiple communication classes
	
                * IPC (boost message queue, currently the most efficient local communication)
                * UnixSocket
                * TCPSocket (remote communication possible)
        * Screen & Color classes

* renderer
	* different renderers for interface with physical or virtual displays
	* multiple renderers can be used at the same time. i.e. SimulatorRenderer(remote over network) && HardwareRenderer
	* currently implemented:
		* RGBMatrixRenderer: hardwareinterface via hzeller rpi-rgb-matrix library
		* SimulatorRenderer: softwareinterface for the CubeSimulator project (see [https://github.com/squarewavedot/CubeSimulator])
		* FPGARendererFTDI: FTDI protocol implementation of FPGA rendering. Meant to be used together with [https://github.com/squarewavedot/ice40-playground.git]
* server & application
	* server logic
	* application interface library (applications link against this)
	* cubeapplication interface with convenient setPixel3D etc. methods

* exampleApplications

* server_* folders:
	* these are main.cpp with setups for servers with the different renderers
	    * server_FPGA_FTDI
	        * the Icebreaker USB FTDI Renderer
	    * server_FPGA_RPISPI
	        * the Icebreaker Raspberry Pi SPI Renderer
		* server_testapp
			* if you have installed OpenCV this target will be available. It shows the Screens as simple OpenCV windows (useful for debugging)
		* server_simulator
			* meant to be used with locally installed simulator (start simulator first)  [https://github.com/squarewavedot/CubeSimulator]

