### Embedding PipeFabric ###

Implementing a program just for a single query isn't very useful - the main
purpose of the PipeFabric framework is to embed stream processing into other
applications. That means that instead of writing results to a file or
to the console as in the example above, result tuples are passed to other
components of your application. This is achieved very easily by using the
`notify` operator which invokes your code (e.g. a lambda function) for each
incoming tuple. Thus, the example above can be modified:

```C++
auto s = t->newStreamFromREST(8099, "^/publish$", RESTSource::POST_METHOD)
  ...
  .notify<ResultTuplePtr>([&](auto tp, bool outdated) {
    std::cout << tp << std::endl;
  });

```
