### Python Integration ###

In addition to the C++ DSL, PipeFabric provides also a Python integration. This
allows to construct topologies (i.e. stream queries) in Python without the need
for writing and compiling C++ code. Due to the nature of Python, this is mainly
intended for prototyping - for applications requiring low latency processing the
native C++ interface should be chosen.

#### Using PipeFabric.Python ####

PipeFabric comes as a single dynamic library `libpfabric` which can be imported
in Python. Similar to the C++ DSL topologies can be created and used to construct
the dataflow graph by adding operators in the dot notation. Because the tuples
processed in such a topology are native Python tuples, lambda functions of the
different operators (e.g. `where`, `map` etc.) can be written directly in Python.

```Python
import pyfabric

t = pyfabric.Topology()
p = t.newStreamFromFile("data.csv") \
     .extract(',') \
     .map(lambda t, o: (int(t[0]), t[1], t[2])) \
     .where(lambda x, o: x[0] > 1) \
     .print()
```

Finally, to start the execution the `start` method of the topology object is invoked.

```Python
t.start()
```
