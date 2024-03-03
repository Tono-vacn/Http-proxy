### C++ Http Cache Proxy

This is a c++ http proxy server based on boost asio. It can cache the http response and serve the cached response to the client. It also supports multi-threading and daemon mode.

### Build

There are two ways to build the project. You can use the makefile or docker.
Clone the project and cd into the project directory.
```bash
# Use makefile
make
./proxy

# Or use docker
cd docker-deploy
sudo docker-compose up --build
# After the build, use the following command to run it as a daemon
sudo docker-compose up -d
```

### Usage

The proxy server listens on port 12345 by default, which is fixed in the source code. You need to set the proxy configuration in your browser to visit the proxy. 

### Features

- [x] Cache the http response(GET only)
- [x] Revalidate the cache
- [x] Multi-threading
- [x] Daemon mode


