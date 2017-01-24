### PipeFabric Operators ###

PipeFabric provides two classes of operators
 * operators (methods of class `Topology`) used to construct a stream of tuples (i.e. a `Pipe`) to initialize a topology,
 * operators (methods of class `Pipe`) which are applied to a stream, i.e. to a `Pipe`.

### Operators for constructing a stream ###

#### newStreamFromFile ####

`Pipe Topology::newStreamFromFile(const std::string& fname)`

This operator reads the file with the given name `fname` and produces a stream of tuples where each line corresponds to one tuple containing only of a single string attribute (`TStringPtr`).

```
Topology t;
auto s = t.newStreamFromFile("data.csv")
```

#### newStreamFromREST ####

`Pipe Topology::newStreamFromREST(p, m, num)`

This is an operator for receiving tuples via REST. Each call of the REST service produces a single tuple (consisting of a single string). The parameters define the URI of the service (`p`), the method (e.g. GET or POST) (`m`) as well as the number of threads started to handle the requests.

#### newStreamFromZMQ ####

`Pipe Topology::newStreamFromZMQ(p, m, src)`

#### newStreamFromTable ####

`Pipe Topology::newStreamFromTable<T, K>(tbl, mode)`

#### fromStream ####

`Pipe Topology::fromStream<T>(stream)`

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

`Pipe Pipe::statefulMap<Tin,Tout,State>(f)`

This operator implements a stateful projection where each tuple of type `Tin` is converted into a tuple of type `Tout` using the function `f` and taking the state into account which can be updated during processing.

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

`extract<T>(c)`

This operator processes a stream of strings, splits each tuple using the given separator character and constructs tuples of type `T`.

#### extractJson ####

`extractJson<T>(tags)`

This operator processes a stream of JSON strings and constructs tuples of type `T`.

#### deserialize ####

`deserialize<T>()`

#### keyBy ####

`keyBy<T,Num,K>()`
`keyBy<T,K>(f)`

assigns the function `f` for deriving (or calculating) a key of type `K` of the input tuples which are of type `T`. Note, that the `K` parameter has the default value of `unsigned long`.

#### assignTimestamps ####

`assignTimestamps<T,Num>()`
 `assignTimestamps<T>(f)`

assigns the function `f` for deriving (or calculating) a timestamp for each input tuple of type `T`.

#### slidingWindow ####

`slidingWindow<T>(w, sz, sd)`
defines a sliding window of the given type and size on the stream. The window type `w` can be row or range for which `sz` specifies the size. In case of a range (time-based) window the `assignTimestamp` operator has to defined before.

#### tumblingWindow ####

`tumblingWindow<T>(w, sz)`

#### queue ####

`Pipe Pipe::queue<T>()`

This operator decouple two operators in the dataflow. The upstream part inserts tuples of type `T` into the queue which is processed by a separate thread to retrieve tuples from the queue and sent them downstream. In this way, the upstream part is not blocked anymore.

#### aggregate ####

`aggregate<Tin, Tout, State>(m, interval)`

calculates a set of aggregates over the stream of type `Tin`, possibly supported by a window. The aggregates a represented by tuples of type `Tout`. `aggregate` needs a helper class of type parameter `State`. The parameters are the mode `m` for triggering the  aggregate calculation, and an optional interval time for producing aggregate values (`interval`).


#### groubBy #####

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

This operator stores tuples from the input stream of type `T` with key type `K` into the table `tbl` and forwards them to its subscribers. `TablePtr` is of type `std::shared_ptr<Table<T, K> >`. Outdated tuples are handled as deletes, non-outdated tuples either as insert (if the key does not exist yet) or update (otherwise). The parameter `autoCommit` determines the autocommit mode.

#### updateTable ####

`updateTable<T, R, K>(tbl, fun)`
