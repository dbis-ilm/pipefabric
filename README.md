### PipeFabric ###

PipeFabric is a DSMS framework for processing streams of tuples where the processing steps are described by queries formulated as dataflow graphs of operators. PipeFabric represents the execution engine by providing only a set of operators and utility classes. It is intended to be used in conjunction with a higher level
dataflow compiler such as Piglet that translates a dataflow specification into C++ code, but can be used
by C++ developers, too.

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

### Usage ###

The main data structure for representing elements of a data stream is the `Tuple` class. `Tuple<>`
represents a template class which can parametrized with the attribute types of the element. Note,
that timestamps are not represented separately: timestamps can be derived from any attribute (or
a combination of attributes). Furthermore, tuples are not copied around but only passed by reference.
For this purpose, the `TuplePtr<>` template is used. Thus, a complete schema definition for a stream
looks like the following:

```
typedef TuplePtr<Tuple<int, std::string, double> > T1;
```

Tuples can be constructed using the `makeTuplePtr` function which of course requires correctly
typed parameters:

```
auto tp = makeTuplePtr(1, std::string("a string"), 2.0);
```

For accessing the individual components of an attribute we provide the `getAttribute<>` template
function where the template parameter is the position of the attribute in the tuple. In order to
access the `string` component of tuple `tp` the following code an be used:

```
auto s = getAttribute<1>(tp);
```

The recommended interface for implementing stream processing pipelines is the `Topology` class
which allows to specify processing steps in a DSL very similar to Apache Spark. The following
code snippet gives an example. See below for an explanation of the provided operators.

```
typedef TuplePtr<Tuple<int, std::string, double> > T1;
typedef TuplePtr<Tuple<double, int> > T2;

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

| Operator | Semantics |
|----------|-----------|
| `newStreamFromFile(fname)` | reads the file with the given name `fname` and produces a stream of tuples where each line corresponds to one tuple containing only of a single string attribute. |
| `newStreamFromREST(p, m, num)` | receives tuples via REST. Each call of the REST service produces a single tuple (consisting of a single string). The parameters define the URI of the service (`p`), the method (e.g. GET or POST) (`m`) as well as the number of threads started to handle the requests.|
| `newStreamFromZMQ(p, m, src)` | |
| `newStreamFromTable<T, K>(tbl, mode)` | |
| `where<T>(f)` | implements a filter operator where all tuples satisfying the predicate  function `f` are forwarded to the next operator. The `T` parameter represents the input/output type (a `TuplePtr` type). |
| `map<Tin,Tout>(f)` | implements a projection where each tuple of type `Tin` is converted into a tuple of type `Tout` using the function `f`. |
| `print<T>(s)` | prints each tuple to the stream s. `T` represents the tuple type. |
| `saveToFile<T>(fname, formatter)` | |
| `extract<T>(c)` | processes a stream of strings, splits each tuple using the given separator character and constructs tuples of type `T`. |
| `deserialize<T>()` | |
| `keyBy<T,K>(f)` | assigns the function `f` for deriving (or calculating) a key of type `K` of the input tuples which are of type `T`. Note, that the `K` parameter has the default value of `unsigned long`. |
| `assignTimestamp<T>(f)` | assigns the function `f` for deriving (or calculating) a timestamp for each input tuple of type `T`. |
| `slidingWindow<T>(w, sz, sd)` | defines a sliding window of the given type and size on the stream. The window type `w` can be row or range for which `sz` specifies the size. In case of a range (time-based) window the `assignTimestamp` operator has to defined before. |
| `tumblingWindow<T>(w, sz)` | |
| `queue<T>()` | decouple two operators in the dataflow. The upstream part inserts tuples of type `T` into the queue which is processed by a separate thread to retrieve tuples from the queue and sent them downstream. In this way, the upstream part is not blocked anymore.|
| `aggregate<Tin, Tout, State>(state, ffinal, fiter, m, interval)` | calculates a set of aggregates over the stream of type `Tin`, possibly supported by a window. The aggregates a represented by tuples of type `Tout`. `aggregate` needs a helper class of type parameter `State`. The parameters are an instance of this state class (`state`), a function to calculate the final aggregates (`ffinal`), a function to update the aggregates (`fiter`), the mode `m` for triggering the  aggregate calculation, and an optional interval time for producing aggregate values (`interval`). |
| `groupBy<>` | |
| `join<>` | |
| `toTable<T,K>(tbl, auto)` |  stores tuples from the input stream of type `T` with key type `K` into the table `tbl` and forwards them to its subscribers. Outdated tuples are handled as deletes, non-outdated tuples either as insert (if the key does not exist yet) or update (otherwise). The parameter `auto` determines the autocommit mode.|

### How to write a query ###

Queries are defined in PipeFabric as dataflow programs in C++, but we provide a DSL simplyfing the formulation.
As an example we want to implement a simple dataflow receiving data via REST, calculating a moving average over the last 10 values, and sending the results to tne console. Let's look at the C++ program.

First, we need to include the PipeFabric header file and should use the namespace.

```
include "pfabric.hpp"

using namespace pfabric;
```

Next, we define the schema: the tuple types for representing input and output data:

```
// the structure of tuples we receive via REST
typedef TuplePtr<Tuple<int, double> > InTuple;

// the structure of our output (aggregate) tuples
typedef TuplePtr<Tuple<double> > ResultTuple;
```

And for the aggregation we have to define a type the captures the aggregation state.

```
// the aggregate operator needs a state object that is defined here:
// template parameters are:
//     * the input type,
//     * the aggregate function (in our case Avg on double values),
//     * the column of the input tuple on which we calculate the aggregate (in
//       our case column #1)
typedef Aggregator1<InTuple, AggrAvg<double, double>, 1> MyAggrState;
```

In the main function we first create a `PFabricContext` object which is needed
to create a new topology. A topology represents a dataflow (i.e. a query) and is
used to add operators. In our example, we start with a REST source for producing
a stream of tuples. In the next step, we extract the `key`and `data` fields and
produce instances of `InTuplePtr`. Then, we add a sliding window and an aggregate
operator with the state class defined before. Finally, we add the print operator
for sending results to `std::cout` and start the topology.

```
int main(int argc, char **argv) {
  PFabricContext ctx;

  auto t = ctx.createTopology();

  auto s = t->newStreamFromREST(8099, "^/publish$", RESTSource::POST_METHOD)
    .extractJson<InTuplePtr>({"key", "data"})
    .slidingWindow<InTuplePtr>(WindowParams::RowWindow, 10)
    .aggregate<InTuplePtr, ResultTuplePtr, MyAggrState> ()
    .print<ResultTuplePtr>(std::cout);

  t->start();
}
```

After starting the program with `./RestDemo` we can send some data via `curl`:

```
curl -H "Content-Type: application/json" \
     -X POST -d '{"key": "xyz", "data": "1.0"}' \
     http://localhost:8099/publish
```

and will receive the output from our stream query.
