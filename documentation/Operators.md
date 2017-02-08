### PipeFabric Operators ###

PipeFabric provides two classes of operators
 * operators (methods of class `Topology`) used to construct a stream of tuples (i.e. a `Pipe`) to initialize a topology,
 * operators (methods of class `Pipe`) which are applied to a stream, i.e. to a `Pipe`.

### Operators for constructing a stream ###

#### newStreamFromFile ####

`Pipe Topology::newStreamFromFile(fname, limit = 0)`

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

`Pipe Topology::newStreamFromREST(port, path, m, num = 1)`

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

`Pipe Topology::newStreamFromZMQ(path, encoding, stype)`

This operator uses the ZeroMQ source to receive tuples via network protocol. The `path` is used to define the network connection endpoint. 
`encoding` describes the encoding of the tuple, available as ASCII or binary. Default is binary mode if not specified. The `stype` defines 
source or sink of ZeroMQ, used as source (publisher) in this context per default.

The following example constructs a ZeroMQ source as a TCP connection at port 5678, waiting for external calls to produce tuples for 
following operators.

```
Topology t;
auto s = t.newStreamFromZMQ("tcp://localhost:5678")
```

#### newStreamFromTable ####

`Pipe Topology::newStreamFromTable<T, K>(tbl, mode)`

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

`Pipe Topology::fromStream<T>(stream)`

This operator gets another already processed stream as input. With this operator it is possible to use another (already preprocessed) 
stream, for example. `T` references to the tuple type, `stream` is another data stream, possibly defined per `toStream()` operator 
explained later on.

The following example defines the structure of a tuple `T1` first, followed by creating a stream in an already existing topology, 
which is then used in `fromStream`.

```
typedef TuplePtr<Tuple<long, std::string, double>> T1;

[...]
          .toStream<T1>(anotherStream);
[...]

Topology t;
auto s = t.fromStream<T1>(anotherStream)
```

#### fromGenerator ####

`Pipe Topology::fromGenerator<T>(gen, num)`

This operator creates tuples by itself, using a generator function `gen` and a parameter `num`. `gen` is a lambda function for creating tuples, `num` is the number of produced tuples.

The following example defines a tuple consisting of two numbers. The generator function produces tuples starting with `0,10` and increasing both numbers by one for all following tuples, described in a lambda function. `streamFromGenerator` uses this function to produce 1000 tuples, forwarding them to the next operators.

```
typedef TuplePtr<Tuple<int, int>> T1;

StreamGenerator<TPtr>::Generator gen ([](unsigned long n) -> TPtr {
    return makeTuplePtr((int)n, (int)n + 10);
});

Topology t;
auto s = t.streamFromGenerator<TPtr>(gen, 1000)
```

### Operators applicable to a stream ###

#### where ####

`Pipe Pipe::where<T>(std::function<bool(const T&, bool)> func)`

This operator implements a filter operator where all tuples satisfying the predicate function `func`
are forwarded to the next operator.
The `T` parameter represents the input/output type (a `TuplePtr` type).

```
typedef TuplePtr<Tuple<int,int,int>> Tin;

Topology t;
auto s = t.newStreamFromFile("data.csv")
          .extract<Tin>(',')
          .where<Tin>([](auto tp, bool) { return get<0>(tp) % 2 == 0; } )
```

#### map ####

`Pipe Pipe::map<Tin,Tout>(std::function<Tout(const Tin&, bool)> func)`

This operator implements a projection where each tuple of type `Tin` (usually a `TuplePtr` type) is converted
into a tuple of type `Tout` using the function `func`.

```
typedef TuplePtr<Tuple<int,int,int>> Tin;
typedef TuplePtr<Tuple<int>> Tout;

Topology t;
auto s = t.newStreamFromFile("data.csv")
          .extract<Tin>(',')
          .map<Tin, Tout>([](auto tp, bool) { return makeTuplePtr(get<1>(tp)); } )
```

#### statefulMap ####

`Pipe Pipe::statefulMap<Tin,Tout,State>(std::function<Tout(const Tin&, bool, std::shared_ptr<State>)> func)`

This operator implements a stateful projection where each tuple of type `Tin` is converted into a tuple of type `Tout` using the 
function `func` and taking the state into account which can be updated during processing. The `state` type can be any class that maintains
a state and is accepted as argument to the map function `func`.

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
          .statefulMap<Tin, Tout, MyState>([](auto tp, bool, std::shared_ptr<MyState> state) { 
              state->cnt++;
              state->sum += get<2>(tp);
              return makeTuplePtr(state->cnt, state->sum); 
          } )
```


#### print ####

`Pipe Pipe::print<T>(std::ostream& s)`

This operator prints each tuple to the stream `s` where the default value for
`s` is `std::cout`. `T` represents the tuple type.

```
Topology t;
auto s = t.newStreamFromFile("data.csv")
          .print<TStringPtr>();
```

#### saveToFile ####

`saveToFile<T>(fname, formatter)`

#### extract #####

`Pipe Pipe::extract<T>(char sep)`

This operator processes a stream of strings, splits each tuple using the given separator character and constructs `sep` tuples of type `T`.

```
typedef TuplePtr<Tuple<int,int,int>> Tin;

Topology t;
auto s = t.newStreamFromFile("data.csv")
          .extract<Tin>(',');
```

#### extractJson ####

`extractJson<T>(tags)`

This operator processes a stream of JSON strings and constructs tuples of type `T`.

#### deserialize ####

`deserialize<T>()`

#### keyBy ####

`keyBy<T,Num,K>()`
`keyBy<T,K>(f)`

assigns the function `f` for deriving (or calculating) a key of type `K` of the input tuples which are of type `T`. Note, 
that the `K` parameter has the default value of `unsigned long`.

#### assignTimestamps ####

`assignTimestamps<T,Num>()`
 `assignTimestamps<T>(f)`

assigns the function `f` for deriving (or calculating) a timestamp for each input tuple of type `T`.

#### slidingWindow ####

`slidingWindow<T>(w, sz, sd)`

defines a sliding window of the given type and size on the stream. The window type `w` can be row or range for which `sz` specifies the size. 
In case of a range (time-based) window the `assignTimestamp` operator has to defined before.

#### tumblingWindow ####

`tumblingWindow<T>(w, sz)`

#### queue ####

`Pipe Pipe::queue<T>()`

This operator decouple two operators in the dataflow. The upstream part inserts tuples of type `T` into the queue which is processed by a 
separate thread to retrieve tuples from the queue and sent them downstream. In this way, the upstream part is not blocked anymore.

#### aggregate ####

`aggregate<Tin, Tout, State>(m, interval)`

calculates a set of aggregates over the stream of type `Tin`, possibly supported by a window. The aggregates a represented by tuples 
of type `Tout`. `aggregate` needs a helper class of type parameter `State`. The parameters are the mode `m` for triggering the 
aggregate calculation, and an optional interval time for producing aggregate values (`interval`).


#### groupBy #####

`groupBy<>`

#### join ####
`join<>`

#### notify ####

`notify<T>`

#### partitionBy ####

`partitionBy<T>(fun, n)`

#### merge ####

`merge<T>()`

#### barrier ####

`barrier<T>`

#### toStream ####

`toStream<T>(stream)`

#### toTable ####

`Pipe Pipe::toTable<T,K>(TablePtr tbl, bool autoCommit)`

This operator stores tuples from the input stream of type `T` with key type `K` into the table `tbl` and forwards them to its 
subscribers. `TablePtr` is of type `std::shared_ptr<Table<T, K> >`. Outdated tuples are handled as deletes, non-outdated tuples 
either as insert (if the key does not exist yet) or update (otherwise). The parameter `autoCommit` determines the autocommit mode.

#### updateTable ####

`updateTable<T, R, K>(tbl, fun)`
