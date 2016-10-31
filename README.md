### PipeFabric ###

PipeFabric is a DSMS framework for processing streams of tuples where the processing steps are described by queries formulated as dataflow graphs of operators. PipeFabric represents the execution engine by providing only a set of operators and utility classes. It is intended to be used in conjunction with a higher level
dataflow compiler such as Piglet that translates a dataflow specification into C++ code, but can be used
by C++ developers, too.


### Tutorials and Documentation ###

 + [Installation](documentation/Installation.md)
 + [Getting started: Using PipeFabric](/documentation/Usage.md)
 + [List of operators](/documentation/Operators.md)
 + [Tutorial: How to write a query](/documentation/Demo.md)
 + [Tutorial: Embedding PipeFabric](/documentation/Embedding.md)
 + [Tutorial: Stream partitioning](/documentation/Partitioning.md)
 + [Tutorial: How to build and use a Docker image](/documentation/Docker.md)
