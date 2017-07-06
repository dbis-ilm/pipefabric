### Installation ###

First of all, you need a very recent C++ compiler supporting the C++14 standard.
PipeFabric relies on several open source components which have to be installed before:

 + [CMake](https://cmake.org/) 3.0 (or newer) build environment
 + [Boost](http://www.boost.org/) 1.61 (or newer) C++ libraries (build all libraries)
 + [ZeroMQ](http://zeromq.org/) socket library (including [zmq.hpp](https://github.com/zeromq/cppzmq/blob/master/zmq.hpp))
 + JeMalloc or TCMalloc library (optional)

There are some additional 3rd party libraries such as [Catch](https://github.com/philsquared/Catch) for testing, 
[SimpleWeb](https://github.com/eidheim/Simple-Web-Server), [Format](https://github.com/fmtlib/fmt), [RocksDB](https://github.com/facebook/rocksdb),
and [JSON](https://github.com/nlohmann/json) but they are either included or downloaded during the build.

After cloning the repository, compile everything:

```
mkdir build; cd build; cmake ../src; make
```

and optionally run the test cases:

```
make test
```
