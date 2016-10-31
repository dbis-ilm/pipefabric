### PipeFabric Operators ###

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
| `aggregate<Tin, Tout, State>(ffinal, fiter, m, interval)` | calculates a set of aggregates over the stream of type `Tin`, possibly supported by a window. The aggregates a represented by tuples of type `Tout`. `aggregate` needs a helper class of type parameter `State`. The parameters are a function to calculate the final aggregates (`ffinal`), a function to update the aggregates (`fiter`), the mode `m` for triggering the  aggregate calculation, and an optional interval time for producing aggregate values (`interval`). |
| `groupBy<>` | |
| `join<>` | |
| `notify<T>` | |
| `barrier<T>` | |
| `toTable<T,K>(tbl, auto)` |  stores tuples from the input stream of type `T` with key type `K` into the table `tbl` and forwards them to its subscribers. Outdated tuples are handled as deletes, non-outdated tuples either as insert (if the key does not exist yet) or update (otherwise). The parameter `auto` determines the autocommit mode.|
