# Matrix Server Troubleshooting Log

## Problem Overview
The matrix server on the Raspberry Pi (`cube.local`) was failing:
**Connectivity Failure**: Example applications cannot connect to the server via TCP port 2017.

---

## Connectivity Issue (In Progress)
Despite the server process running, applications cannot establish a connection to port 2017.

### Current Investigation:
- **Process Status**: `ps aux` confirms `matrix_server` is running as root on the Pi.
- **Port Status**: `netstat -tpln` and `ss -tulpn` show that **nothing is listening on port 2017**.
- **Log Analysis**: 
    - The server logs show it is "creating default config" and "writing matrixServerConfig.json".
    - **Critical Missing Entry**: The log `[Server] Start accepting on address: 0.0.0.0 and port: 2017` (expected from `TcpServer.cpp`) is **not** appearing in the output.
- **Logic Check**: 
    - The `TcpServer` constructor calls `doAccept()`, which should log the message and bind the socket.
    - Since the log is missing, the `TcpServer` object might not be initializing correctly, or the `boost::asio::io_service::run()` thread is failing to start/execute the task.
    - The `ioThread` is started at the end of the `Server` constructor.

### Next Steps:
1. Verify if `TcpServer` is actually instantiated and not throwing an early exception.
2. Investigate potential deadlocks in the `Server` constructor or `ioContext.run()` call.
