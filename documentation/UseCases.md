## Use Cases for PipeFabric ##

Here we want to describe and show some use cases for PipeFabric to give you an insight about the possibilities
and necessary efforts of utilization. Therefore we provide the following topics:

 + DEBS2017, a research challenge for detecting anomalies in RDF streaming data
 + Image Processing, realized by matrix operations (coming soon)
 + Processing of Movement Trajectories, tracking movement of different objects (coming soon)

To run them on your own, we give the necessary simple installation steps below because the use cases are not
built by default in PipeFabric. Besides this, an additional summary of the most important facts about the topics
above are given.


### Installation ###

First, you can build PipeFabric as described in the [Installation description](documentation/Installation.md),
running the test cases if everything works. To enable the building of use cases, you have to switch a CMake
variable called `BUILD_USE_CASES` to `ON`. You can directly edit the `CMakeLists.txt` file or pass the command
`-DBUILD_USE_CASES=ON` when building (e.g. `cmake -DBUILD_USE_CASES=ON ../src;`). Remember to delete the
`CMakeCache.txt` first, or the change in the variable will not be recognized!

After the build is finished, you can run the use cases independently.

 + DEBS2017: In your build folder, just run "./usecases/DEBS2017/debs2017" on your command line.


### DEBS2017 ###

#### Challenge description ####

The [DEBS2017 Grand Challenge](https://project-hobbit.eu/challenges/debs-grand-challenge/) is a yearly reoccuring
series, facing different problems of processing data streams. The objective of the challenge in 2017 was to
process and analyze RDF streaming data given by digital and analogue sensors of manufacturing equipment. Anomalies
measured by the sensors should be detected as final result.

As limiting conditions, the following constraints are given:

The data of the sensors is first properly clustered. For the clusters, state transitions between them over time
are observed (modeled as a Markov chain). Anomalies are then detectable as different sequences of transitions
that are not very common, expressed by a probability lower than a certain threshold.

Besides the difficulties of realizing and updating such a Markov chain without missing some anomalies later on
the classification step, the challenge lies in providing a high throughput and low latency processing. The data
from sensors could be real time streaming, therefore the anomaly detection has to be fast for reacting in short
time to the individual anomaly as well as being efficient enough to allow parallel processing of multiple sensor
inputs.

#### Solution with PipeFabric ####

There are four datasets available for testing purposes. They have just differences in their size as well as in
their amount of anomalies. We deliver the second dataset for the use case (which has 10 MB in size), but the
solution works also for bigger datasets.

First, the given metadata of the challenge is read from file and stored in appropriate data structures. After that
the stream of RDF data is started, ready for processing. The stored information in RDF (like timestamps or the
machine identifier) is extracted and forwarded as well-structured tuple. An additional window operator is
responsible for only regarding tuples up to a certain age.

The following clustering along with updating existing clusters is realized next in a single customized operator.
Therefore a clustering state is stored and updated accordingly when the next tuple arrives. After the clustering,
the Markov chain (responsible for finding anomalies) receives the cluster data, implemented also as a single
customized operator. If the transition probability is under the given threshold, an anomaly is found and returned
by a simple print on the console.

For data of real sensors with real manufacturing machines, this print could also be connected to a warning or
whatever the reaction to an anomaly should be.


### Image Processing ###

(coming soon!)


### Movement Trajectories ###

(coming soon!)
