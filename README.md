### PipeFabric ###

PipeFabric is a DSMS framework for processing streams of tuples where the processing steps are described by queries formulated as dataflow graphs of operators. PipeFabric represents the execution engine by providing only a set of operators and utility classes. It is intended to be used in conjunction with a higher level
dataflow compiler such as Piglet that translates a dataflow specification into C++ code, but can be used
by C++ developers, too.

### Installation ###

PipeFabcric relies on several open source components which have to be installed before:

 + CMake 3.0 (or newer) build environment
 + Boost 1.61 (or newer) C++ libraries (build all libraries)
 + ZeroMQ socket library (including zmq.hpp from https://github.com/zeromq/cppzmq/blob/master/zmq.hpp)
 + JeMalloc or TCMalloc library (optional)

After cloning the repository, compile everything:

```
mkdir build; cd build; cmake ../src; make
```

and optionally run the test cases:

```
make test
```

### Usage ###

The recommended interface for implementing stream processing pipelines is the `Topology` class
which allows to specify processing steps in a DSL very similar to Apache Spark. The following
code snippet gives an example. See below for an explanation of the provided operators.

```
Topology t;
auto s = t.newStreamFromFile("file.csv")
  .extract<T1>(',')
  .where<T1>([](auto tp, bool outdated) { return getAttribute<0>(tp) % 2 == 0; } )
  .map<T1,T2>([](auto tp) -> T2 {
    return makeTuplePtr(getAttribute<2>(tp), getAttribute<0>(tp));
  })
  .print<T2>(std::cout);

t.start();
```
