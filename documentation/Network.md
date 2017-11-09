## Additional network sources ##

There already exist ZeroMQ and REST as network sources, providing tuples via network connection.
In addition, the AMQP (Advanced Message Queuing Protocol) used by RabbitMQ as well as the Apache
Kafka protocol can be used as source. However, there are additional libraries/installs necessary to
run the protocol which are not delivered in PipeFabric per default.

## RabbitMQ ##

### Preliminaries and Installation ###

For AMQP (RabbitMQ):
 + [RabbitMQ Server](https://www.rabbitmq.com/download.html)
 + [Amqpcpp library](https://github.com/akalend/amqpcpp), available on GitHub

The server is necessary for realization of AMQP, while the Amqpcpp library allows using the server
within the C++ language. On Linux side, the server is usually inside of the standard repositories,
so you can easily install it with the command `sudo apt-get install rabbitmq-server`.

After the RabbitMQ server and the Amqpcpp library are installed and added to your path (in a way
that CMake can find them), you have to enable the RabbitMQ source for PipeFabric by switching the
CMake variable `USE_RABBITMQ` to `ON`. This can be done manually in the CMakeLists.txt file in the
src folder or by passing `-DUSE_RABBITMQ=ON` to cmake, like `cmake -DUSE_RABBITMQ=ON ../src`.

In addition, you have to start the RabbitMQ server before running the test case. This can be done
on console by the command `service rabbitmq-server start`. Else the test case will throw an error,
namely `AMQP cannot create socket`.

### Usage ###

PipeFabric provides an interface to the RabbitMQ server in which all currently available messages
on the server are gathered, transformed to tuples and forwarded to the query. This is done by using
the operator `newStreamFromRabbitMQ`:

`Pipe<TStringPtr> newStreamFromRabbitMQ(const std::string& info, const std::string& queueName)`

Each incoming message of the RabbitMQ service produces a single tuple (consisting of a single
string). The parameter `info` declares the connection of the server. Usually when the RabbitMQ
server is started without modifications, a user named "guest" with the password "guest" is applied
to the system. The `info` parameter can then be entered as `guest:guest@localhost:5672`, where 5672
is the used port. The parameter `queueName` describes the queue, where messages are exchanged.

The operator currently checks once if there are messages (tuples) available on the RabbitMQ server.
If yes, all the messages are gathered and sent downstreams to the subscribers (that means, the
following operator(s)). Then it finishes. However, it can be easily adapted to stay waiting,
repeatedly asking the server if new messages have arrived.

## Apache Kafka ##

### Preliminaries and Installation ###

For Apache Kafka:
 + [Apache Zookeeper](https://zookeeper.apache.org/)
 + [Apache Kafka server](https://kafka.apache.org/downloads)
 + [Librdkafka](https://github.com/edenhill/librdkafka), C++ Kafka library, available on GitHub
 + [Cppkafka](https://github.com/mfontanini/cppkafka), C++ wrapper around Librdkafka, available on GitHub

The Kafka server is used for exchanging messages and uses Apache Zookeeper as dependency. On most
Linux systems, the Zookeeper is available in the standard repository, so it is possible to use the
command `sudo apt-get install zookeeperd` for installing. For setting up the Kafka server on Linux
inside your home directory, you can simply do the following on the command line (using wget):

```
$ mkdir -p ~/kafka
$ cd ~/kafka
$ wget http://www-us.apache.org/dist/kafka/0.10.2.0/kafka_2.10-0.10.2.0.tgz
$ tar xvzf kafka_2.10-0.10.2.0.tgz --strip 1
$ ./bin/kafka-server-start.sh ~/kafka/config/server.properties
```

For deleting topics in Apache Kafka, you should edit the server properties located in
`~/kafka/config/server.properties`, removing the `#` in the line `#delete.topic.enable=true`.

The library `librdkafka` provides support for C++ when using Apache Kafka. The wrapper `cppkafka`
uses the library, providing a much more userfriendly utilization. Both have to be installed in a
way that CMake can find them (libraries and headers).

Apache Kafka is then enabled in PipeFabric by switching the CMake variable `USE_KAFKA` to `ON`.
This can be done manually in the CMakeLists.txt file in the src folder or by passing
`-DUSE_KAFKA=ON` to cmake, like `cmake -DUSE_KAFKA=ON ../src`.

In addition, you have to start the Kafka server before running the test case. This can be done
on console inside the Kafka folder by the command
`./bin/kafka-server-start.sh ./config/server.properties`. Else the test case will throw an error,
namely `Connection refused - brokers are down`.

### Usage ###

PipeFabric provides an interface to the Kafka server in which all currently available messages
on the server are gathered, transformed to tuples and forwarded to the query. This is done by using
the operator `newStreamFromKafka`:

`Pipe<TStringPtr> Topology::newStreamFromKafka(const std::string& broker, const std::string& topic, const std::string& groupID)`

Each incoming message of the Kafka server produces a single tuple (consisting of a single string).
The parameter `broker` describes a cluster instance on the server, possible to use your localhost.
The `topic` is the topic on which the data is exchanged, respectively tuples. Finally, the
`groupID` of the consumer describes to which group (of producers and consumers) it belongs to.
Kafka automatically destroys groups that have no members left.

The operator currently checks once if there are messages (tuples) available on the Kafka server.
If yes, all the messages are consecutively gathered and sent downstreams to the subscribers (that
means, the following operator(s)). Then it finishes. However, it can be easily adapted to stay
waiting, repeatedly asking the server if new messages have arrived.

