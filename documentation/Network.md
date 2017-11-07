## Additional network sources ##

There already exist ZeroMQ and REST as network sources, providing tuples via network connection. 
In addition, the AMQP (Advanced Message Queuing Protocol) used by RabbitMQ can be used as source. 
However, there are two additional libraries/installs necessary to run the protocol which are not 
delivered in PipeFabric per default: 

 + [RabbitMQ Server](https://www.rabbitmq.com/download.html)
 + [Amqpcpp library](https://github.com/akalend/amqpcpp), available on GitHub

The server is necessary for realization of AMQP, while the Amqpcpp library allows using the server 
within the C++ language. On Linux side, the server is usually inside of the standard repositories, 
so you can easily install it with the command `sudo apt-get install rabbitmq-server`.


### Installation ###

After the RabbitMQ server and the Amqpcpp library are installed and added to your path (in a way 
that CMake can find them), you have to enable the network source by switching the CMake variable 
`NETWORK_SOURCES` to `ON`. This can be done manually in the CMakeLists.txt file in the src folder 
or by passing `-DNETWORK_SOURCES=ON` to cmake, like `cmake -DNETWORK_SOURCES=ON ../src`.

In addition, you have to start the RabbitMQ server before running the test case. This can be done 
on console by the command `$service rabbitmq-server start` (without the $). Else the test case 
will throw an error.


#### newStreamFromRabbitMQ ####

`Pipe<TStringPtr> Topology::newStreamFromRabbitMQ(const std::string& info)`

This is an operator for receiving tuples via RabbitMQ. Each incoming message of the RabbitMQ 
service produces a single tuple (consisting of a single string). The parameter `info` declares the 
connection of the server. Usually when the RabbitMQ server is started without modifications, a 
user named "guest" with the password "guest" is applied to the system. The `info` parameter can 
then be entered as `guest:guest@localhost:5672`, where 5672 is the used port.

The operator checks once if there are messages (tuples) available on the RabbitMQ server. If yes, 
all the messages are gathered and sent downstreams to the subscribers (that means, the following 
operator(s)). Then it finishes. However, it can be easily adapted to stay waiting, repeatedly 
asking the server if new messages have arrived.
