### Partitioning for multithreaded processing ###

PipeFabric provides two basic operators for supporting stream partitioning on
a multicore machine: `partitionBy` splits a stream into sub-streams which are sent
to different instances of the subsequent operators in the dataflow and `merge` combines
the resulting substreams into a single one. In the following example a stream
is created from a file and after transforming the tuples the stream is split into
5 sub-streams based on the given partitioning function. Hence, the `where` and `map`
operators are processing in 5 separate threads on the partitions of the stream, which
are finally merged into a single stream again.

```C++
auto s = t->newStreamFromFile("data.csv")
  .extract<Tin>(',')
  .partitionBy([](auto tp) { return get<0>(tp) % 5; }, 5)
  .where([](auto tp, bool outdated) { return get<0>(tp) % 2 == 0; } )
  .map<Tout>([](auto tp) { return makeTuplePtr(get<0>(tp)); } )
  .merge()
  .print(std::cout);
```
