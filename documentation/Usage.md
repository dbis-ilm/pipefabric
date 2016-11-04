### Usage ###

The main data structure for representing elements of a data stream is the `Tuple` class. `Tuple<>`
represents a template class which can parametrized with the attribute types of the element. Note,
that timestamps are not represented separately: timestamps can be derived from any attribute (or
a combination of attributes). Furthermore, tuples are not copied around but only passed by reference.
For this purpose, the `TuplePtr<>` template is used. Thus, a complete schema definition for a stream
looks like the following:

```C++
typedef TuplePtr<Tuple<int, std::string, double> > T1;
```

Tuples can be constructed using the `makeTuplePtr` function which of course requires correctly
typed parameters:

```C++
auto tp = makeTuplePtr(1, std::string("a string"), 2.0);
```

For accessing the individual components of an attribute we provide the `get<>` template
function where the template parameter is the position of the attribute in the tuple. In order to
access the `string` component of tuple `tp` the following code an be used:

```C++
auto s = get<1>(tp);
```

The recommended interface for implementing stream processing pipelines is the `Topology` class
which allows to specify processing steps in a DSL very similar to Apache Spark. The following
code snippet gives an example. See below for an explanation of the provided operators.

```C++
typedef TuplePtr<Tuple<int, std::string, double> > T1;
typedef TuplePtr<Tuple<double, int> > T2;

Topology t;
auto s = t.newStreamFromFile("file.csv")
  .extract<T1>(',')
  .where<T1>([](auto tp, bool outdated) { return get<0>(tp) % 2 == 0; } )
  .map<T1,T2>([](auto tp) -> T2 {
    return makeTuplePtr(get<2>(tp), get<0>(tp));
  })
  .print<T2>(std::cout);

t.start();
```
