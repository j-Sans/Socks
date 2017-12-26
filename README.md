# Socks
### A convinient TCP C-Socket wrapper class.

Two easy-to-use classes wrapping the functionality of C-Sockets. These objects can be easily called to send data back and forth, and do all of the nasty C standard library for the user.

## Usage

There are two classes, ServerSocket and ClientSocket. A single ServerSocket object can simultaneously connect with multiple clients.

### ClientSocket

The following is an example usage of a ClientSocket object.
```C++
ClientSocket client("localhost", 3000);
client.send("Hello server!");
std::cout << "Client received " << client.receive();
```

The constructor (or ``` setSocket(...)```) takes in the name of the host as a string as well as the port on which to run. That port must be free or else an error will occur. Messages can be sent and read using ```send(...)``` and ```receive(...)```.

The socket can also be closed so the port can be reused. The socket is returned to an unset state and ```set(...)``` must be called again before it can be used.

A limit for the amount of time a socket listens for a message can also be set using ```setTimeout(...)```.

More detailed documentation is available at [ClientSocket.hpp](https://github.com/ja-San/Socks/blob/master/C-Sockets/ClientSocket.hpp).

### ServerSocket
The following is an example usage of a ClientSocket object.
```C++
ServerSocket server(3000, 1);
server.addClient();
server.send("Hello client!", 0);
std::cout << "Server received " << server.receive(0);
```

The constructor (or ``` setSocket(...)```) takes in the port on which to run as well as the maximum number of connections that can be made. That port must be free or else an error will occur. To add clients, ```addClient()``` must be called. Unless clients are removed with ```closeConnection(...)```, it can only be called up to the number of times specified by the maximum number of connections. Messages can be sent and read using ```send(...)``` and ```receive(...)```. They work like the client, except the take as a parameter the index of the client with whom to correspond. ```broadcast(...)``` sends a message to each client connectioned. Each socket is given the next available index. This means that unless a low-index client is disconnected, the newest client will have the greatest index.

A limit for the amount of time a socket listens for a message can also be set using ```setTimeout(...)```.  ```setHostTimeout(...)``` does the same, except for server actions, such as listening for new clients.

More detailed documentation is available at [ServerSocket.hpp](https://github.com/ja-San/Socks/blob/master/C-Sockets/ServerSocket.hpp).

### Credits
I have written all of the code present, but have consulted with [Ben Spector](https://github.com/sydriax) for the future  direction of this project.
