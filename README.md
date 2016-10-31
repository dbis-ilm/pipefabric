### PipeFabric ###

PipeFabric is a C++ framework for processing streams of tuples where the
processing steps are described by queries formulated as dataflow graphs of
operators. PipeFabric represents the execution engine by providing a set of
operators and utility classes. It consists of the following main components:

  + a publish-subscribe framework optimized for low-latency processing within a single machine
  + a library of operators for data stream processing including aggregates, grouping, and joins as
    well as complex event processing
  + a basic DSL for specifying dataflows (called topologies) in C++   


### Tutorials and Documentation ###

 + [Installation](documentation/Installation.md)
 + [Getting started: Using PipeFabric](/documentation/Usage.md)
 + [List of operators](/documentation/Operators.md)
 + [Tutorial: How to write a query](/documentation/Tutorial.md)
 + [Tutorial: Embedding PipeFabric](/documentation/Embedding.md)
 + [Tutorial: Stream partitioning](/documentation/Partitioning.md)
 + [Tutorial: How to build and use a Docker image](/documentation/Docker.md)
