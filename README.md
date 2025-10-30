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
Note: If no port is specified, a default port of 5000 is used.
#### Run Server 
```
./netcopy server
./netcopy server "port"
./netcopy server 5000
```

#### Run Client
```
./netcopy client "host/IP" "port"
./netcopy client 127.0.0.1 5000
```
#### Run Proxy
```
./netcopy proxy "port"
./netcopy proxy 5000
```
#### Run HTTP Proxy
Set your browser to use the proxy server. The default IP and port is 127.0.0.1 and 8080.
<br>
<br> Note: If you set this as the HTTP proxy for your browser, you might also have to configure HTTPS requests to use this proxy as well.
```
./netcopy http-proxy
./netcopy http-proxy "port"
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