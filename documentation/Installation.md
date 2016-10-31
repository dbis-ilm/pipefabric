### Installation ###

First of all, you need a very recent C++ compiler supporting the C++14 standard.
PipeFabric relies on several open source components which have to be installed before:

 + CMake 3.0 (or newer) build environment
 + Boost 1.61 (or newer) C++ libraries (build all libraries)
 + ZeroMQ socket library (including zmq.hpp from https://github.com/zeromq/cppzmq/blob/master/zmq.hpp)
 + JeMalloc or TCMalloc library (optional)

There are some additional 3rd party libraries such as SimpleWeb, Format, and JSON but
they are downloaded during the build.

After cloning the repository, compile everything:

```
mkdir build; cd build; cmake ../src; make
```

and optionally run the test cases:

```
make test
```
