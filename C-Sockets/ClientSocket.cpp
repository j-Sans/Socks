#include "ClientSocket.hpp"

ClientSocket::ClientSocket() {}

void ClientSocket::setSocket(const char* hostName, int portNum) {
    this->portNumber = portNum;
    
    int returnVal;
    
    addrinfo hints; //A struct containing information on the address. Will be passed to getaddrinfo() to give hints about the connection to be made
    addrinfo* serverAddressList; //A pointer to an addrinfo struct that will be filled with the server address by getaddrinfo()
    
    memset(&hints, 0, sizeof(hints)); //Set hints
    hints.ai_family = AF_UNSPEC; //Can be either IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; //TCP Socket
    
    /* getaddrinfo
     The getaddrinfo() fills a given addrinfo struct with the relevant information. The return value is nonzero when there is an error.
     
     The first argument is the name of the host to connect with.
     
     The second argument is a string representation of the port number.
     
     The third argument is a pointer to an addrinfo struct which contains hints about the type of connection to be made.
     
     The fourth argument is a pointer which will be filled with a linked list of hosts returned. If just one host is being looked for, the first result can just be used.
     */
    returnVal = getaddrinfo(hostName, std::to_string(portNum).c_str(), &hints, &serverAddressList);
    
    //Check for an error with getaddrinfo()
    if (returnVal != 0) {
        throw strcat((char *)"ERROR getting host address: ", gai_strerror(returnVal)); //gai_strerror() returns a c string representation of the error
    }
    
    //Set the first host in the list to the desired host
    addrinfo serverAddress = *serverAddressList;
    
    /* socket()
     The socket() function returns a new socket, with three parameters.
     
     The first argument is address of the domain of the socket.
     The two possible address domains are the unix, for commen file system sockets, and the internet, for anywhere on the internet.
     AF_UNIX is generally used for the former, and AF_INET generally for the latter.
     
     The second argument is the type of the socket.
     The two possible types are a stream socket where characters are read in a continuous stream, and a diagram socket, which reads in chunks.
     SOCK_STREAM is generally used for the former, and SOCK_DGRAM for the latter.
     
     The third argument is the protocol. It should always be 0 except in unusual circumstances, and then allows the operating system to chose TCP or UDP, based on the socket type. TCP is chosen for stream sockets, and UDP for diagram sockets
     
     The function returns an integer than can be used like a reference to the socket. Failure results in returning -1.
     */
    //In this case, the values are taken from getaddrinfo(), from the first returned addrinfo struct in the linked list.
    this->connectionSocket = socket(serverAddress.ai_family, serverAddress.ai_socktype, serverAddress.ai_protocol);
    
    //Checks for errors initializing socket
    if (socket < 0)
        throw strcat((char *)"ERROR opening socket: ", strerror(errno));
    
    //No need to call bind() (see server side) because the local port number doesn't matter; the kernel will find an open port.
    
    /* connect()
     The connect() function is called by the client to establish a connection with the server, with three arguments.
     
     The first argument is the small integer reference for the socket.
     
     The second argument is the address of the host to connect to, including the port number, all in the form of a sockaddr struct.
     
     The third argument is the size of the address.
     
     The function returns 0 if successful and -1 if it fails.
     */
    if (connect(this->connectionSocket, serverAddress.ai_addr, serverAddress.ai_addrlen) < 0)
        throw strcat((char *)"ERROR connecting: ", strerror(errno));
    
    freeaddrinfo(serverAddressList); //Free the linked list now that we have the local host information
    
    this->setUp = true;
}

std::string ClientSocket::send(const char* message, bool ensureFullStringSent) {
    if (!this->setUp)
        throw "Socket not set";
    
    if (std::string(message) == "")
        throw "No message to send";
    
    unsigned long messageLength = strlen(message);
    
    long sentSize = write(this->connectionSocket, message, messageLength);
    
    if (sentSize < 0) {
        throw strcat((char *)"ERROR sending message: ", strerror(errno));
    } else if (sentSize < messageLength) {
        std::string extraStr; //Holds the rest of the string that was not sent
        for (unsigned long a = sentSize; a < messageLength; a++) {
            extraStr += message[a];
        }
        if (ensureFullStringSent) {
            this->send(extraStr.c_str(), ensureFullStringSent); //Recursively keep sending the rest of the string until it is all sent
            return "";
        }
        return extraStr; //Return any part of the string that was not sent. This occurs if the string is too long
    }
    return "";
}

std::string ClientSocket::receive(bool* socketClosed) {
    if (!this->setUp)
        throw "Socket not set";
    
    //Initialize the buffer where received info is stored
    bzero(this->buffer, BUFFER_SIZE);
    
    long messageSize; //Stores the return value from the calls to read() and write() by holding the number of characters either read or written
    
    /* read()
     The read() function will read in info from the client socket, with three arguments. It will block until the client writes and there is something to read in.
     
     The first argument is the reference for the client's socket.
     
     The second argument is the buffer to store the message.
     
     The third argument is the maximum number of characters to to be read into the buffer.
     */
    messageSize = read(this->connectionSocket, this->buffer, BUFFER_SIZE);
    
    //Checks for errors reading from the socket
    if (messageSize < 0)
        throw strcat((char *)"ERROR reading from socket: ", strerror(errno));
    
    if (socketClosed != nullptr && messageSize == 0) {
        *socketClosed = true;
    }
    
    return std::string(this->buffer, messageSize);
}

bool ClientSocket::getSet() {
    return this->setUp;
}

ClientSocket::~ClientSocket() {
    if (this->setUp) {
        //Properly terminate the sockets
        try {
            close(this->connectionSocket);
        } catch (...) {
            printf("Error closing socket (client side)");
        }
    }
}
