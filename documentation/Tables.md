### Persistent Tables ###

Though, PipeFabric is a framework for data stream processing, it supports also the concept of
tables to store data persistently. Tables can be used either as source of data streams (i.e. updates),
as persistent sink of data streams or to manage the state of operators in a persistent way.
Currently, two different table implementations are supported (both with the same interface):

 + hashmap-based in-memory table,
 + disk-based table using RocksDB for managing data.

The table implementation is chosen at compile time by setting the option `USE_ROCKSDB_TABLE` in
`src/CMakeLists.txt`.

#### Creating a Table ####

Tables have to be created explicitly before use by specifying the schema in the form of a
`Tuple` type passed a template argument to the `createTable` method. In the following example,
a table `TEST_TBL` with three columns is created:

```C++
typedef Tuple<int, std::string, double> RecordType;

PFabricContext ctx;
auto myTable = ctx.createTable<RecordType, int>("TEST_TBL");
```
Note, that a key has to be specified for each table and the data type of the key (in the example above
`int`)  is passed as second argument to the template instantiation. Furthermore, in contrast to type
arguments for the streaming operators, the `createTable` method requires a `Tuple` type but not a `TuplePtr`.
The key is always part of the tuple type.

There is an alternative way of creating a new table that allows to specify a complete schema consisting
of column types and names using a `TableInfo` object. This approach is needed if ad-hoc queries on tables
have to be supported.

```C++
...
TableInfo tblInfo("TEST_TBL",
      { ColumnInfo("col1", ColumnInfo::Int_Type),
        ColumnInfo("col2", ColumnInfo::String_Type),
        ColumnInfo("col3", ColumnInfo::Double_Type) },
auto myTable = ctx.createTable<RecordType, int>(tblInfo);
```

#### Accessing a Table ####

Tables in PipeFabric support different ways of storing, updating, and retrieving data:

 + via the Table API, i.e. insert, deleteByKey, deleteWhere, updateByKey, ...
 + via the streaming operator DSL, i.e. toTable, newStreamFromTable, updateTable, ...
 + via the ad-hoc query facility.

The Table API provides basic functions to insert, update, delete, and retrieve table records. In the following list
`KeyType` refers to the data type of the key, `RecordType` to the tuple type representing the schema.

 + `insert(KeyType k, const RecordType& rec)` inserts a new record with the given key into the table.
 + `deleteByKey(KeyType k)` deletes the record with the given key.
 + `deleteWhere(std::function<bool(const RecordType&)> predicate)` deletes all records satisfying the given predicate.
 + `updateByKey(KeyType k,  std::function<void(RecordType&)> updater)` updates the tuple with the given key by applying 
   the function `updater` which accepts the tuple as parameter and modified it in a certain way.
 + `updateWhere(std::function<bool(const RecordType&)> predicate, std::function<void(RecordType&)> updater)` updates all tuples from
   the table satisfying the given predicate by applying the function `updater` which modifies the tuple.
 + `getByKey(KeyType k)` returns a pointer to the tuple with the given key in the form of `TuplePtr<RecordType>`. If no tuple 
    exists for this key, then an exception is raised.
 + `select(std::function<bool(const RecordType&)> predicate)`

Streaming operators are used as part of constructing a topology. Table-related operators are 
(see [Operators](Operators.md) for a detailed description):

 + `newStreamFromTable` constructs a new data stream from updates on the given table. Whenever a
    tuple in the table is updated, a new stream element is constructed and published.
 + `selectFromTable` is used to perform a standard (batch) query on a table by selecting all
    tuples satisfying an optionally specified predicate. Though, this constructs also a data stream,
    the stream ends when the last tuple in the table is reached.
 + `toTable` writes the elements of a data stream to the given table, either inserting new tuple,
    updating existing tuples (identified by the key) or deleting tuples from the table (in case
    of outdated tuples).
 + `updateTable` allows to use stream elements to update a table. It differs from `toTable` by allowing
    to perform arbitrary updates via a user-provided update function. In contrast, `toTable` stores tuples
    of the stream directly, i.e. the schema of the table and the stream have to be the same.

