### PipeFabric Operators ###

PipeFabric provides two classes of operators
 * operators (methods of class `Topology`) used to construct a stream of tuples (i.e. a `Pipe`) to initialize a topology,
 * operators (methods of class `Pipe`) which are applied to a stream, i.e. to a `Pipe`.

### Operators for constructing a stream ###

#### newStreamFromFile ####

`Pipe<TStringPtr> Topology::newStreamFromFile(fname, limit = 0)`

This operator reads the file with the given name `fname` and produces a stream of tuples where each line corresponds to one tuple
only consisting of a single string attribute (`TStringPtr`). There is an optional variable to define the maximum tuples to be read,
which is zero on default (that means, read until end of file).

The following example reads a file named `data.csv`. It has to be in the same folder as the runnable code, else use
"\path\to\your\data.csv" to find it.

```
Topology t;
auto s = t.newStreamFromFile("data.csv")
```

#### newStreamFromREST ####

`Pipe<TStringPtr> Topology::newStreamFromREST(port, path, m, num = 1)`

This is an operator for receiving tuples via REST. Each call of the REST service produces a single tuple (consisting of a single string).
The parameters define the TCP port for receiving REST calls (`port`), the URI path of the service (`path`), the method
(e.g. GET or POST) (`m`) as well as the number of threads (`num`) started to handle the requests (which is one per default).

The following example listens on port 8099 with corresponding interface and posting-method. An external call produces a single tuple,
which is forwarded to the next operator (usually an extract-operator, defined later on).

```
Topology t;
auto s = t.newStreamFromREST(8099, "^/publish$", RESTSource::POST_METHOD)
```

#### newStreamFromZMQ ####

`Pipe<TStringPtr> Topology::newAsciiStreamFromZMQ(path, stype)`
`Pipe<TBufPtr> Topology::newBinaryStreamFromZMQ(path, stype)`

This operator uses the ZeroMQ source to receive tuples via network protocol. The `path` is used to define the network connection endpoint.
The `stype` defines source or sink of ZeroMQ, used as source (publisher) in this context per default. Ascii or Binary are two types of encoding
the tuples, choose correspondingly.

The following example constructs a ZeroMQ source as a TCP connection at port 5678, waiting for external calls to produce tuples for
following operators.

```
Topology t;
auto s = t.newAsciiStreamFromZMQ("tcp://localhost:5678")
```

#### newStreamFromTable ####

`Pipe<T> Topology::newStreamFromTable(tbl, mode)`

This operator generates tuples based on updates of a table. For every update on `tbl` the corresponding tuple is created.
`T` defines the tuple type, `K` is the key type. `tbl` is the table on which updates are monitored and `mode` is an optional
parameter to trigger on updates immediately (default) or only after the transaction commits.

The following example defines first the structure of a tuple `T1` per typedef (same structure as the tuples in the table `testTable`).
For every update on `testTable` a tuple with structure `T1` and keytype `long` (key based on first attribute) is forwarded to
following operators.

```
typedef TuplePtr<Tuple<long, std::string, double>> T1;

Topology t;
auto s = t.newStreamFromTable<T1, long>(testTable)
```

#### fromStream ####

`Pipe<T> Topology::fromStream(stream)`

This operator gets another already processed stream as input. With this operator it is possible to use another (already preprocessed)
stream, for example. `T` references to the tuple type, `stream` is another data stream, possibly defined per `toStream()` operator
explained later on.

The following example defines the structure of a tuple `T1` first, along with a new stream element. Then, tuples are pushed into this
stream by an already existing topology. Finally, the stream is then used in `fromStream`.

```
typedef TuplePtr<Tuple<long, std::string, double>> T1;

PFabricContext ctx;
Dataflow::BaseOpPtr stream = ctx.createStream<T1>("streamName");

[...]
          .toStream(streamName);
[...]

Topology t;
auto s = t.fromStream<T1>(streamName)
```

#### fromGenerator ####

`Pipe<T> Topology::streamFromGenerator(gen, num)`

This operator creates tuples by itself, using a generator function `gen` and a parameter `num`. `gen` is a lambda function for creating
tuples, `num` is the number of produced tuples.

The following example defines a tuple consisting of two numbers. The generator function produces tuples starting with `0,10` and increasing
both numbers by one for all following tuples, described in a lambda function. `streamFromGenerator` uses this function to produce 1000 tuples,
forwarding them to the next operators.

```
typedef TuplePtr<Tuple<int, int>> T1;

StreamGenerator<T1>::Generator gen ([](unsigned long n) -> T1 {
    return makeTuplePtr((int)n, (int)n + 10);
});

Topology t;
auto s = t.streamFromGenerator<T1>(gen, 1000)
```

### Operators applicable to a stream ###

#### extract #####

`Pipe<Tout> Pipe::extract(char sep)`

This operator processes a stream of strings, splits each tuple using the given separator character `sep` and constructs tuples of type `T`.

The following example reads tuples from a file called "data.csv" and extracts tuples out of it, consisting out of three integer attributes.

```
typedef TuplePtr<Tuple<int,int,int>> Tin;

Topology t;
auto s = t.newStreamFromFile("data.csv")
          .extract<Tin>(',');
```

#### extractJson ####

`Pipe<Tout> Pipe::extractJson(const std::initializer_list<std::string>& keys)`

This operator processes a stream of JSON strings and constructs tuples of type `T`. The given `keys` are needed for reading JSON strings.

The following example reads JSON strings from REST source and extracts them into tuples `Tin`, consisting out of an integer (key) and double
(data).

```
typedef TuplePtr<Tuple<int, double>> Tin;

auto s = t.newStreamFromREST(8099, "^/publish$", RESTSource::POST_METHOD)
          .extractJson<Tin>({"key", "data"})
```

#### where ####

`Pipe<T> Pipe::where(typename Where<T>::PredicateFunc func)`

This operator implements a filter operator where all tuples satisfying the predicate function `func` are forwarded to the next operator.
The `T` parameter represents the input/output type (a `TuplePtr` type).

The following example reads again tuples from a file called "data.csv". After extracting them, all tuples whose first attribute is uneven
(mod 2) are dropped.

```
typedef TuplePtr<Tuple<int,int,int>> Tin;

Topology t;
auto s = t.newStreamFromFile("data.csv")
          .extract<Tin>(',')
          .where([](auto tp, bool) { return get<0>(tp) % 2 == 0; } )
```

#### map ####

`Pipe<Tout> Pipe::map(typename Map<T, Tout>::MapFunc func)`

This operator implements a projection where each tuple of type `T` (usually a `TuplePtr` type) is converted into a tuple of type `Tout` using
the function `func`.

The following example reads tuples again from "data.csv". Each tuple consists out of three integer attributes. After the `map` operator, each
tuple only consists out of one integer attribute (the second attribute from inserted tuple).

```
typedef TuplePtr<Tuple<int,int,int>> Tin;
typedef TuplePtr<Tuple<int>> Tout;

Topology t;
auto s = t.newStreamFromFile("data.csv")
          .extract<Tin>(',')
          .map<Tout>([](auto tp, bool) { return makeTuplePtr(get<1>(tp)); } )
```

#### statefulMap ####
`Pipe<Tout> Pipe::statefulMap(typename StatefulMap<T, Tout, State>::MapFunc func)`

This operator implements a stateful projection where each tuple of type `T` is converted into a tuple of type `Tout` using the function `func`
and taking the state into account which can be updated during processing. The `state` type can be any class that maintains a state and is accepted
as argument to the map function `func`.

The following example calculates the number of already processed tuples and a sum over the third attribute of input tuples. First, a state is
defined, containing the mentioned tuple count and sum. After reading and extracting tuples from file, the state is updated everytime through
the `statefulMap` operator, returning tuple count and sum to next operators.

```
typedef TuplePtr<Tuple<int,int,int>> Tin;
typedef TuplePtr<Tuple<int, int>> Tout;

struct MyState {
        MyState() : cnt(0), sum(0) {}
        int cnt, sum;
};

Topology t;
auto s = t.newStreamFromFile("data.csv")
          .extract<Tin>(',')
          .statefulMap<Tout, MyState>([](auto tp, bool, std::shared_ptr<MyState> state) { 
              state->cnt++;
              state->sum += get<2>(tp);
              return makeTuplePtr(state->cnt, state->sum); 
          } )
```

#### print ####
`Pipe<T> Pipe::print(std::ostream& os, ffun)`

This operator prints each tuple to the stream `s` where the default value for `s` is `std::cout`.

The following example prints all tuples (three integer attributes each) from file to console (per std::cout).

```
typedef TuplePtr<Tuple<int,int,int>> Tin;

Topology t;
auto s = t.newStreamFromFile("data.csv")
          .extract<Tin>(',')
          .print();
```

#### saveToFile ####
`Pipe<T> Pipe::saveToFile(fname, ffun)`

This operator saves incoming tuples into a file named `fname`. `ffun` is an optional function for formatting them.

The following example reads tuples from "data.csv", selecting only tuples whose first attribute is even, and saves them to "resultFile.txt".

```
typedef TuplePtr<Tuple<int,int,int>> Tin;

Topology t;
auto s = t.newStreamFromFile("data.csv")
          .extract<Tin>(',')
          .where([](auto tp, bool) { return get<0>(tp) % 2 == 0; } )
          .saveToFile("resultFile.txt");
```

#### deserialize ####

`Pipe<Tout> Pipe::deserialize<Tout>()`

This operator deserializes tuples from byte arrays. A byte array is represented by the type `TBufPtr` and usually produced by a ZMQ source
operator in binary mode. Thus, `deserialize` is mainly used in combination with the `newBinaryStreamFromZMQ` operator. The target type of
deserialization is given by the `Tout` parameter.

#### keyBy ####

`Pipe<T> Pipe::keyBy(std::function<KeyType(const T&)> extractor)`
`Pipe<T> Pipe::keyBy<N>()`

This operator assigns the function `extractor` for deriving (or calculating) a key of type `K` of the input
tuples which are of type `T`. Note that the `KeyType` parameter has the default value of `unsigned long`.

The second version of the `keyBy` operator uses the N-th column as the key. No separate key extractor function has to be specified.

The following two examples show the use of the `keyBy` operator, doing the same (with different syntax). The key of incoming tuples is set
to first attribute (integer) and can now be used for following functions, like a join or grouping.

```
typedef TuplePtr<Tuple<int,int,int>> Tin;

Topology t;
auto s = t.newStreamFromFile("data.csv")
          .extract<Tin>(',')
          .keyBy<int>([](auto tp) { return get<0>(tp); })
```

```
typedef TuplePtr<Tuple<int,int,int>> Tin;

Topology t;
auto s = t.newStreamFromFile("data.csv")
          .extract<Tin>(',')
          .keyBy<0>()
```

#### assignTimestamps ####

`Pipe<T> Pipe::assignTimestamps(typename Window<T>::TimestampExtractorFunc func)`
`Pipe<T> Pipe::assignTimestamps<N>()`

This operator assigns a timestamp to incoming tuples, used for window or outdated calculations. The first method uses `func` on tuple type `T` as
a function to calculate the corresponding timestamp. The second method uses a specified column `N` (a number between 0 and NumColumns - 1) as timestamp.

The following two examples show the usage of this operator (with same results on different syntax), both setting the second attribute as timestamp.

```C++
typedef TuplePtr<Tuple<int,std::string,int>> Tin;

Topology t;
auto s = t.newStreamFromFile("data.csv")
          .extract<Tin>(',')
          .assignTimestamps([](auto tp) { return get<1>(tp); })
```

```C++
typedef TuplePtr<Tuple<int,std::string,int>> Tin;

Topology t;
auto s = t.newStreamFromFile("data.csv")
          .extract<Tin>(',')
          .assignTimestamps<1>()
```

#### slidingWindow ####

`Pipe<T> Pipe::slidingWindow(wt, sz, ei)`

defines a sliding window of the given type and size on the stream. The window type `w` can be row or range for which `sz` specifies the size. 
In case of a range (time-based) window the `assignTimestamp` operator has to defined before.

#### tumblingWindow ####

`Pipe<T> Pipe::tumblingWindow(wt, sz)`

#### queue ####

`Pipe<T> Pipe::queue()`

This operator decouple two operators in the dataflow. The upstream part inserts tuples of type `T` into the queue which is processed by a 
separate thread to retrieve tuples from the queue and sent them downstream. In this way, the upstream part is not blocked anymore.

#### aggregate ####

`Pipe<Tout> Pipe::aggregate<Tout, State>(tType, tInterval)`
`Pipe<Tout> Pipe::aggregate<Tout, State>(finalFun, iterFun, tType, tInterval)`

calculates a set of aggregates over the stream of type `Tin`, possibly supported by a window. The aggregates a represented by tuples 
of type `Tout`. `aggregate` needs a helper class of type parameter `State`. The parameters are the mode `m` for triggering the 
aggregate calculation, and an optional interval time for producing aggregate values (`interval`).
As long as only standard aggregate functions (e.g. sum, count, avg etc.) are used the state class can be implemented
using the predefined `AggregatorN` template where `N` denotes the number of aggregate functions. The following
example creates a state class for computing the average, the count, and the sum on a stream of tuples conisting
of `int` values:

```C++
typedef TuplePtr<Tuple<int>> InTuple;
typedef Aggregator3<
  InTuple, i        // the input type for the aggregation
  AggrAvg<int, int>,// an aggregate for calculating the average (input column type = int, output type = int)
  0,                // the values to be aggregated are taken from column 0
  AggrCount<int>,   // an aggregate for counting values (result type = int)
  0,                // again, we count values in column 0
  AggrSum<int>      // an aggregate for calculating the sum of int values
  0                 // column 0 again
  > AggrState;
```

With the help of this state class, `aggregate` can be used. However, we have to define the ouput type of the
aggregate which consists in this case of three `int` values (for avg, count, sum):

```C++
tyepdef TuplePtr<Tuple<int, int, int>> OutTuple;

Topology t;
auto s = t.newStreamFromFile("data.csv")
          .extract<InTuple>(',')
          .aggregate<OutTuple, AggrState>()
          ...
```

In case you need more advanced aggregations going beyond the standard aggregates you still can implement your
own aggregation state class. This class has to provide an `iterate` function that is called for each input
tuple and a `finalize` function for producing the final result tuple.

#### groupBy #####

`Pipe<Tout> Pipe::groupBy(tType, tInterval)`
`Pipe<Tout> Pipe::groupBy(finalFun, iterFun, tType, tInterval)`

#### join ####
`Pipe<typename SHJoin<T, T2, KeyType>::ResultElement> Pipe::join(otherPipe, pred)`

#### notify ####

`Pipe<T> Pipe::notify(std::function<void(const T&, bool)> func, std::function<void(const PunctuationPtr&)> pfunc)`

Notify is an operator for triggering callbacks. It forwards all tuples to its subscribers and invokes 
the callback function `func` for each tuple as well as the (optional) punctuation callback `pfunc` for each punctuation
tuple. Note that the tuple cannot be modified for the stream (use `map` instead) and beware potential race 
conditions if you modify a global state in the callback functions.

```C++
typedef TuplePtr<Tuple<int,std::string,int>> Tin;

Topology t;
auto s = t.newStreamFromFile("data.csv")
          .extract<Tin>(',')
          .notify(auto tp, bool outdated) {
            auto v = get<0>(tp);
            std::cout << "---> " << v << std::endl;
          });
```

#### partitionBy ####

`Pipe<T> Pipe::partitionBy(std::function<PartitionID(const T&)> pFun, unsigned int nPartitions)`

The `partitionBy` operator partitions the input stream on given partition id which is derived 
using a user-defined function `pFun` and forwards the tuples of each partition to a subquery. 
Subqueries are registered via their input channels for each partition id. The number of paritions
is specified by the parameter `nPartitions`. If a `partitionBy` operator is inserted into the dataflow
then a separate subquery is constructed for each partition containing the dataflow from the operator
following partitionBy to the next `merge` operator. Thus, everything between `partitionBy` and `merge`
runs in parallel (i.e. in separate threads). The purpose of the partitioning function `pFun` is the
return a partition id (an integer value ranging from 0 to `nPartitions-1`) for each incoming tuple.

In the following example the stream is splitted into 5 partitions using a simple hash partitioning scheme
on the first column of the tuples. Thus, 5 instances of the `where` operator are created and run in parallel.
The final `merge` step combines the partitioned stream into a single stream.

```C++
Topology t;
  auto s = t.newStreamFromFile("file.csv")
    .extract<T1>(',')
    .partitionBy([](auto tp) { return get<0>(tp) % 5; }, 5)
    .where([](auto tp, bool outdated) { return get<0>(tp) % 2 == 0; } )
    .merge()
```

#### merge ####

`Pipe<T> Pipe::merge()`

The `merge` operator is used only in combination with `partitionBy` to combine the partitioned stream into
a single one. See above for an example.

#### barrier ####

`Pipe<T> Pipe::barrier(cVar, mtx, f)`

#### batch ####

`Pipe<BatchPtr<T>> Pipe::batch(size_t bsize)`

The `batch` operator combines `bsize` consecutive tuples of the input stream into a single batch of tuples
that is forwarded to the subscribers as a single tuple. A batch is defined as follows

```C++
template <typename T>
using BatchPtr = TuplePtr<Tuple<std::vector<std::pair<T, bool>>>>;
```

where `T` denotes the type of the input tuple and the pairs represent the tuple as well as its outdated flag.
In the following example batches of 100 tuples are constructed:

```C++
auto s = t.newStreamFromFile("file.csv")
    .extract<T1>(',')
    .map<T2>([](auto tp, bool outdated) -> T2 { return makeTuplePtr(get<0>(tp)); } )
    .batch(100)
```

#### toStream ####

`Pipe<T> Pipe::toStream(stream)`

#### sendZMQ ####

`Pipe<T> Pipe::sendZMQ(path, stype, mode)`

#### matchByNFA ####

`Pipe<Tout> Pipe::matchByNFA(nfa)`

#### matcher ####

`Pipe<Tout> Pipe::matcher(expr)`

#### toTable ####

`Pipe<T> Pipe::toTable(tbl, autoCommit)`

This operator stores tuples from the input stream of type `T` with key type `K` into the table `tbl` and forwards them to its 
subscribers. `TablePtr` is of type `std::shared_ptr<Table<T, K> >`. Outdated tuples are handled as deletes, non-outdated tuples 
either as insert (if the key does not exist yet) or update (otherwise). The parameter `autoCommit` determines the autocommit mode.

#### updateTable ####

`Pipe<T> Pipe::updateTable(tbl, updateFunc)`

