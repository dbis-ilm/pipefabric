### SQL Query Compiler ###

PipeFabric provides an experimental processor for SQL queries. A SQL query can be executed on tables
that are filled by stream queries. For this purpose, the query string is parsed and compiled into a
C++ program (i.e. a DLL) which is then dynamically loaded and executed. Due to rather long C++ compilation
times this takes some time.

Using the query compiler is quite simple: just use the `QueryCompiler` class:

```
PFabricContext ctx;
QueryCompiler sqlCompiler;
sqlCompiler.readSettings(library_path);

auto plan = sqlCompiler.execQuery(ctx, "select * from SENSOR_DATA");
```

Note that the table referenced in the FROM clause has to be created before using `ctx.createTable(...)` and
that this table requires an explicit schema (via `TableInfo`). Furthermore, before compiling a query the
`readSettings` method should be called to load a file `config.ini` with definitions for the C++ compiler
and libraries.

For each SQL query, the query compiler creates a C++ file as illustrated for the example query shown above:

```
#include "qcomp/TopologyBuilder.hpp"

using namespace pfabric;

BUILDER_CLASS(Query_2)

typedef TuplePtr<Tuple<int, double>> Tuple_1_Type_;

PFabricContext::TopologyPtr Query_2::create(PFabricContext& ctx) {
        auto SENSOR_DATA = ctx.getTable<Tuple_1_Type_, int>("SENSOR_DATA");
        topology = ctx.createTopology();
        topology->selectFromTable<Tuple_1_Type_, int>(SENSOR_DATA)
                .print<Tuple_1_Type_>();
        return topology;
}

extern "C" BOOST_SYMBOL_EXPORT Query_2 Query_2_obj_;
Query_2 Query_2_obj_;
```
