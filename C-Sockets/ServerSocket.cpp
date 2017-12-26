#include "ServerSocket.hpp"

ServerSocket::ServerSocket() {}

ServerSocket::ServerSocket(int portNum, int maxConnections) {
    this->setSocket(portNum, maxConnections);
}

//Static functions

std::string ServerSocket::getHostName() {
    char name[1024];
    if (gethostname(name, 1024) < 0) { //Fill name with the host name. Then, if an error occurred, throw an exception
        throw std::runtime_error(strcat((char *)"ERROR getting host name: ", strerror(errno)));
    }
    return std::string(name);
}

//Public member functions

void ServerSocket::setSocket(int portNum, int maxConnections) {
    if (this->setUp)
        throw std::logic_error("Socket already set");
    
    int returnVal;
    
    for (int a = 0; a < maxConnections; a++) {
        this->activeConnections.push_back(false); //All connections initially inactive
        this->clientSocketsFD.push_back(0); //No socket file descriptors set
        this->clientAddresses.push_back(sockaddr_storage()); //All addresses set as empty structs
        this->clientAddressSizes.push_back(socklen_t()); //All address sizes set as empty sizes
    }
    
    addrinfo hints; //A struct containing information on the address. Will be passed to getaddrinfo() to give hints about the connection to be made
    addrinfo* serverAddressList; //A pointer to an addrinfo struct that will be filled with the server address by getaddrinfo()
    
    memset(&hints, 0, sizeof(hints)); //Initializes hints with all 0's
    hints.ai_family = AF_UNSPEC; //Can be either IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; //TCP Socket
    hints.ai_flags = AI_PASSIVE; //Set the socket to accept connections (as server socket)
    
    /* getaddrinfo
     The getaddrinfo() fills a given addrinfo struct with the relevant information. The return value is nonzero when there is an error.
     
     The first argument is the name of the host to connect with.
     
     The second argument is a string representation of the port number.
     
     The third argument is a pointer to an addrinfo struct which contains hints about the type of connection to be made.
     
     The fourth argument is a pointer which will be filled with a linked list of hosts returned. If just one host is being looked for, the first result can just be used.
     */
    returnVal = getaddrinfo(NULL, std::to_string(portNum).c_str(), &hints, &serverAddressList);
    
    //Throw an error if there is an error getting the local address
    if (returnVal != 0) throw std::runtime_error(strcat((char *)"ERROR getting local address: ", gai_strerror(returnVal))); //gai_strerror() returns a c string representation of the error
    
    //Set the first host in the list to the desired host
    this->serverAddress = *serverAddressList;
    
    /* socket()
     The socket() function returns a new socket, with three parameters.
     
     The first argument is address of the domain of the socket.
     The two possible address domains are the unix, local communication, and internet, for communication across the internet.
     AF_UNIX is generally used for the former, and AF_INET and AF_INET6 generally for the latter.
     
     The second argument is the type of the socket.
     The two possible types are a stream socket where characters are read in a continuous stream, and a diagram socket, which reads in chunks.
     SOCK_STREAM is generally used for the former, and SOCK_DGRAM for the latter.
     
     The third argument is the protocol. It should be 0 except in unusual circumstances, and then allows the operating system to chose TCP or UDP, based on the socket type. TCP is chosen for stream sockets, and UDP for diagram sockets
     
     The function returns an integer than can be used like a reference to the socket. Failure results in returning -1.
     */
    //In this case, the values are taken from getaddrinfo(), from the first returned addrinfo struct in the linked list.
    this->hostSocketFD = socket(this->serverAddress.ai_family, this->serverAddress.ai_socktype, this->serverAddress.ai_protocol);
    
    //Checks for errors initializing socket
    if (socket < 0) throw std::runtime_error(strcat((char *)"ERROR opening socket", strerror(errno)));
    
    int enable = 1;
    //This code tells the kernel that the port can be reused as long as there isn't an active socket listening there. This means that after the socket is closed the port can immediately be reused without giving an error
    if (setsockopt(this->hostSocketFD, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1) {
        throw std::runtime_error(strcat((char *)"ERROR setting port to reusable", strerror(errno)));
    }
    
    /* bind()
     The bind() function connects a socket to a local address, with three parameters.
     Here it will connect the socket to the (local) host at the proper port number.
     
     The first argument is the socket, by its simple integer reference.
     
     The second argument is a pointer to the address, contained as a sockaddr_in struct. It must be cast properly.
     
     The third argument is the size of the sockaddr_in struct that is passed in by reference.
     
     This function can fail for multiple reasons. The most likely one is if the port is already in use.
     */
    if (bind(this->hostSocketFD, this->serverAddress.ai_addr, this->serverAddress.ai_addrlen) < 0) {
        throw std::runtime_error(strcat((char *)"ERROR binding host socket to local port: ", strerror(errno)));
    }
    
    /* listen()
     The listen() function listens for connections on the socket, with two arguments.
     
     The first argument is the socket, by its file descriptor (simple integer reference).
     
     The second argument is the size of the "backlog queue", or the number of connections that can be waiting as another connection is handled. It basically is the number of connections that can wait before being accepted.
     
     This function cannot fail, as long as the socket is valid.
     */
    if (listen(this->hostSocketFD, 1) < 0) {
        throw std::runtime_error(strcat((char *)"ERROR listening for incoming connections", strerror(errno)));
    }
    
    freeaddrinfo(serverAddressList); //Free the linked list now that we have the local host information
    
    this->setUp = true; //All functions ensure the socket has been set before doing anything
}

void ServerSocket::addClient() {
    if (!this->setUp)
        throw std::logic_error("Socket not set");
    
    int nextIndex = this->getNextAvailableIndex(); //Find the next available index at which to set the new connection
    
    if (nextIndex == -1) {
        throw std::logic_error(strcat((char *)"Max number of sockets: ", std::to_string(this->activeConnections.size()).c_str()));
    }
    
    /* accept()
     The accept() function block the current thread until a connection is formed between the client and the server, with three arguments. It then wakes when the connection is successfully established.
     
     The first argument is the host side socket, passed by its file descriptor.
     
     The second argument is a reference to the address of the client, in the form of a sockaddr struct pointer.
     
     The third argument is the size of the struct, passed by value.
     
     The return value is a socket, passed by a small integer reference.
     */
    this->clientSocketsFD[nextIndex] = accept(this->hostSocketFD, (struct sockaddr *)&this->clientAddresses[nextIndex], &this->clientAddressSizes[nextIndex]);
    
    //Checks for error with accepting
    if (this->clientSocketsFD[nextIndex] < 0)
        throw std::runtime_error(strcat((char *)"ERROR accepting client", strerror(errno)));
    
    this->activeConnections[nextIndex] = true;
}

void ServerSocket::closeConnection(unsigned int clientIndex) {
    if (!this->setUp)
        throw std::logic_error("Socket not set");
    
    //Throw an error if there is no socket at the index to close
    if (clientIndex >= this->activeConnections.size() || !this->activeConnections[clientIndex])
        throw std::logic_error("Socket index uninitialized");
    
    //Close the socket of the given index
    close(this->clientSocketsFD[clientIndex]);
    
    //Reset the information for the closed socket
    this->clientAddresses[clientIndex] = sockaddr_storage();
    this->clientAddressSizes[clientIndex] = 0;
    this->activeConnections[clientIndex] = false;
}

std::string ServerSocket::send(const char* message, unsigned int clientIndex, bool ensureFullStringSent) {
    if (!this->setUp)
        throw std::logic_error("Socket not set");
    
    //A blank message won't be sent
    if (std::string(message) == "")
        throw std::logic_error("No message to send");
    
    //Throw an error if there is no socket at the index
    if (clientIndex >= this->activeConnections.size() || !this->activeConnections[clientIndex])
        throw std::logic_error("Socket index uninitialized");
    
    unsigned long messageLength = strlen(message);
    
    long sentSize = write(this->clientSocketsFD[clientIndex], message, messageLength);
    
    if (sentSize < 0) {
        throw std::runtime_error(strcat((char *)"ERROR sending message: ", strerror(errno)));
    } else if (sentSize < messageLength) { //If only some of the string was sent, return what wasn't sent (or send the rest)
        std::string extraStr; //Holds the rest of the string that was not sent
        for (unsigned long a = sentSize; a < messageLength; a++) {
            extraStr += message[a];
        }
        if (ensureFullStringSent) { //This optional bool is set to false. If manually changed to true send the rest. Otherwise, return the unsent part
            this->send(extraStr.c_str(), clientIndex, ensureFullStringSent); //Recursively keep sending the rest of the string until it is all sent
            return "";
        }
        return extraStr; //Return any part of the string that was not sent. This occurs if the string is too long
    }
    return ""; //Full string was sent
}

void ServerSocket::broadcast(const char* message, bool ensureFullStringSent) {
    if (!this->setUp)
        throw std::logic_error("Socket not set");
    
    //Send the message to each active client
    for (int a = 0; a < this->activeConnections.size(); a++) {
        if (this->activeConnections[a]) {
            this->send(message, a, ensureFullStringSent);
        }
    }
}

std::string ServerSocket::receive(unsigned int clientIndex, bool* socketClosed) {
    if (!this->setUp)
        throw std::logic_error("Socket not set");
    
    //Throw an error if there is no socket at the index from which to receive
    if (clientIndex >= this->activeConnections.size() || !this->activeConnections[clientIndex])
        throw std::logic_error("Socket index uninitialized");
    
    //Initialize the buffer where received info is stored
    bzero(this->buffer, BUFFER_SIZE);
    
    long messageSize; //Stores the return value from the calls to read() and write() by holding the number of characters either read or written
    
    /* read()
     The read() function will read in info from the client socket, with three arguments. It will block the thread until the client writes and there is something to read in.
     
     The first argument is the reference for the client's socket.
     
     The second argument is the buffer to store the message.
     
     The third argument is the maximum number of characters to to be read into the buffer.
     */
    messageSize = read(this->clientSocketsFD[clientIndex], this->buffer, BUFFER_SIZE);
    
    //Checks for errors reading from the socket
    if (messageSize < 0)
        throw std::runtime_error(strcat((char *)"ERROR reading from socket: ", strerror(errno)));
    
    //A blank message indicates that the socket has closed from the client side. If this is the case, close the connection.
    if (socketClosed != nullptr && messageSize == 0) {
        *socketClosed = true;
        this->closeConnection(clientIndex);
    }
    
    return std::string(this->buffer, messageSize);
}

bool ServerSocket::receivedFromAll(const char* messageToCompare) {
    if (!this->setUp)
        throw std::logic_error("Socket not set");
    
    //If each client sent the same message, return true
    for (int a = 0; a < this->activeConnections.size(); a++) {
        bool connectionClosed = false;
        if (this->activeConnections[a] && this->receive(a, &connectionClosed) != messageToCompare) { //Also closes sockets that closed on client side
            return false;
        }
    }
    return true;
}

void ServerSocket::setTimeout(unsigned int seconds, unsigned int milliseconds) {
    if (!this->setUp)
        throw std::logic_error("Socket not set");
    
#if defined(_WIN32)
    DWORD timeout = (seconds * 1000) + milliseconds;
    for (int a = 0; a < this->activeConnections.size(); a++) {
        if (this->activeConnections[a]) setsockopt(this->clientSocketsFD[a], SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    }
#else
    struct timeval time; //Time holds the maximum time to wait
    time.tv_sec = seconds;
    time.tv_usec = (milliseconds * 1000);
    
    for (int a = 0; a < this->activeConnections.size(); a++) { //For each socket, set the maximum waiting time the inputted amount
        /* setsockopt()
         The setsockopt() function changes options for a given socket, depending on the parameters.
         
         The first argument is the client side socket, passed by its file descriptor.
         
         The second argument is ...?
         
         The third argument tells what setting to change. SO_RCVTIMEO indicates the socket option is for the amount of time to wait while timing out.
         */
        if (this->activeConnections[a]) setsockopt(this->clientSocketsFD[a], SOL_SOCKET, SO_RCVTIMEO, (struct timeval*)&time, sizeof(struct timeval));
    }
#endif
}

void ServerSocket::setHostTimeout(unsigned int seconds, unsigned int milliseconds) {
    if (!this->setUp)
        throw std::logic_error("Socket not set");
    
#if defined(_WIN32)
    DWORD timeout = (seconds * 1000) + milliseconds;
    setsockopt(this->hostSocketFD, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
#else
    struct timeval time;
    time.tv_sec = seconds;
    time.tv_usec = (milliseconds * 1000);
    setsockopt(this->hostSocketFD, SOL_SOCKET, SO_RCVTIMEO, (struct timeval*)&time, sizeof(struct timeval));
#endif
}

unsigned int ServerSocket::numberOfClients() const {
    if (!this->setUp)
        throw std::logic_error("Socket not set");
    
    //Count the number of connections which are active
    int connections = 0;
    for (int a = 0; a < this->activeConnections.size(); a++) {
        if (this->activeConnections[a]) {
            connections++;
        }
    }
    return connections;
}

bool ServerSocket::isSet() const {
    return this->setUp;
}

//Private member functions

int ServerSocket::getNextAvailableIndex() const {
    if (!this->setUp)
        throw std::logic_error("Socket not set");
    
    for (int a = 0; a < this->activeConnections.size(); a++) {
        if (!this->activeConnections[a]) return a;
    }
    return -1;
}

//Destructor

ServerSocket::~ServerSocket() {
    if (this->setUp) {
        //Properly terminate the sockets on both client and host side
        for (int clientIndex = 0; clientIndex < this->activeConnections.size(); clientIndex++) {
            if (this->activeConnections[clientIndex]) {
                try {
                    close(this->clientSocketsFD[clientIndex]);
                } catch (...) {
                    printf("Error closing client socket (server side)");
                }
            }
        }
        try {
            close(this->hostSocketFD);
        } catch (...) {
            printf("Error closing host server socket (server side)");
        }
    }
}
