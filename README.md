## Building

### Prerequisites
- C++ compiler
- CMake >3.21

### Building with CMake
Configure build directory:
```
cmake --preset release
```

Build:
```
cmake --build build
```

### Running netcopy
<br>Note: If no port is specified, a default port of 5000 is used.
Run Server 
```
./netcopy server
./netcopy server "port"
```

Run Client
```
./netcopy client "host/IP" "port"
./netcopy client 127.0.0.1 5000
```
Run Proxy
```
./netcopy proxy "port"
./netcopy proxy 5000
```

### Client Commands
Clear Terminal
```
clear
```
Exit Terminal
```
exit
```
Get File From Server
```
get test.txt
get "fileName"
```
Put File Onto Server
```
put test.txt
get "test.txt"