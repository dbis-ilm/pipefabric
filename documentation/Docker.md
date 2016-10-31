### Docker ###

For your convenience we provide a docker image with all the necessary libraries
and a precompiled PipeFabric. This Dockerfile is based on Dan Liew cxxdev image.
Simply download the Dockerfile and run

```
docker build -t cxxdev .
```

After some time needed to fetch all packages, building Boost, CMake, ZMQ, and
PipeFabric you can start working with

```
docker run -i -t cxxdev /bin/bash
```
