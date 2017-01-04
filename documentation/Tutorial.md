### How to write a query ###

Queries are defined in PipeFabric as dataflow programs in C++, but we provide a DSL simplyfing the formulation.
As an example we want to implement a simple dataflow receiving data via REST, calculating a moving average over the last 10 values, and sending the results to tne console. Let's look at the C++ program.

First, we need to include the PipeFabric header file and should use the namespace.

```C++
#include "pfabric.hpp"

using namespace pfabric;
```

Next, we define the schema: the tuple types for representing input and output data:

```C++
// the structure of tuples we receive via REST
typedef TuplePtr<Tuple<int, double> > InTuplePtr;

// the structure of our output (aggregate) tuples
typedef TuplePtr<Tuple<double> > ResultTuplePtr;
```

And for the aggregation we have to define a type the captures the aggregation state.

```C++
// the aggregate operator needs a state object that is defined here:
// template parameters are:
//     * the input type,
//     * the aggregate function (in our case Avg on double values),
//     * the column of the input tuple on which we calculate the aggregate (in
//       our case column #1)
typedef Aggregator1<InTuplePtr, AggrAvg<double, double>, 1> MyAggrState;
```

In the main function we first create a `PFabricContext` object which is needed
to create a new topology. A topology represents a dataflow (i.e. a query) and is
used to add operators. In our example, we start with a REST source for producing
a stream of tuples. In the next step, we extract the `key`and `data` fields and
produce instances of `InTuplePtr`. Then, we add a sliding window and an aggregate
operator with the state class defined before. Finally, we add the print operator
for sending results to `std::cout` and start the topology. The default mode of
starting is asynchronously, therefore we invoke `wait()` to wait until the execution
is finished (which will never happen in this example).

```C++
int main(int argc, char **argv) {
  PFabricContext ctx;

  auto t = ctx.createTopology();

  auto s = t->newStreamFromREST(8099, "^/publish$", RESTSource::POST_METHOD)
    .extractJson<InTuplePtr>({"key", "data"})
    .slidingWindow<InTuplePtr>(WindowParams::RowWindow, 10)
    .aggregate<InTuplePtr, ResultTuplePtr, MyAggrState> ()
    .print<ResultTuplePtr>(std::cout);

  t->start();
  t->wait();
}
```

After starting the program with `./RestDemo` we can send some data via `curl`:

```C++
curl -H "Content-Type: application/json" \
     -X POST -d '{"key": "10", "data": "1.0"}' \
     http://localhost:8099/publish
```

and will receive the output from our stream query.
