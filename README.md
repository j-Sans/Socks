# Socks
### A convinient TCP C-Socket wrapper class.

Two easy-to-use classes wrapping the functionality of C-Sockets. These objects can be easily called to send data back and forth, and they do all of the nasty C standard library for the programmer.

## Usage

There are two classes, ServerSocket and ClientSocket. A single ServerSocket object can simultaneously connect with multiple ClientSocket objects.

### ClientSocket

The following is an example usage of a ClientSocket object connecting with a Server on the local host.
```C++
ClientSocket client("localhost", 3000);
client.send("Hello server!");
std::cout << "Client received " << client.receive();
```

The constructor (or ``` setSocket(const char* hostName, int portNum)```) takes in the name of the host as a string as well as the port on which to run. That port must be free or else an error will occur. For getting the host name, see ServerSocket below. Messages can be sent and read using ```send(const char* message)``` and ```receive()```.

The socket can also be closed with ```close()``` so the port can be reused. The socket is returned to an unset state and it must be set before it can be used.

A limit for the amount of time a socket listens for a message can also be set using ```setTimeout(unsigned int seconds, unsigned int milliseconds = 0)```.

More detailed documentation is available at [ClientSocket.hpp](https://github.com/ja-San/Socks/blob/master/C-Sockets/ClientSocket.hpp).

### ServerSocket
The following is an example usage of a ServerSocket object.
```C++
ServerSocket server(3000, 1);
server.addClient();
server.send("Hello client!", 0);
std::cout << "Server received " << server.receive(0);
```

The constructor (or ``` setSocket(int portNum, int maxConnections))```) takes in the port on which to run as well as the maximum number of connections that can be made. That port must be free or else an error will occur. To add clients, ```addClient()``` must be called. Unless clients are removed with ```closeConnection(unsigned int clientIndex)```, it can only be called up to the number of times specified by the maximum number of connections. Messages can be sent and read using ```send(const char* message, unsigned int clientIndex)``` and ```receive(unsigned int clientIndex)```. They work like the client, except the take as a parameter the index of the client with whom to correspond. Each socket is given the next available index. This means that unless a low-index client is disconnected, the newest client will have the greatest index. ```broadcast(const char* message)``` sends a message to each client connectioned, and therefore needs no client index. 

A limit for the amount of time a socket listens for a message can also be set using ```setTimeout(unsigned int seconds, unsigned int milliseconds = 0)```.  ```setHostTimeout(unsigned int seconds, unsigned int milliseconds = 0)``` does the same, except for server actions, such as listening for new clients.

To get the name of the host, call the static function ```ServerSocket::getHostName()```.

More detailed documentation is available at [ServerSocket.hpp](https://github.com/ja-San/Socks/blob/master/C-Sockets/ServerSocket.hpp).

### Credits
I have written all of the code present, but have consulted with [Ben Spector](https://github.com/sydriax) for the future  direction of this project.
