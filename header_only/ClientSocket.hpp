#ifndef ClientSocket_hpp
#define ClientSocket_hpp

#include <iostream>
#include <string>
#include <exception>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define BUFFER_SIZE 65535

class ClientSocket {
public:
    //Constructor
    ClientSocket() {}
    ClientSocket(const char* hostName, int portNum) {
        this->setSocket(hostName, portNum);
    }
    
    //Destructor
    ~ClientSocket() {
        if (this->setUp) {
            //Properly terminate the sockets
            try {
                ::close(this->connectionSocket);
            } catch (...) {
                printf("Error closing socket (client side)");
            }
        }
    }
    
    //Public member functions
    
    /*!
     * A function to initialize the socket. This must be done before the socket can be used. Will throw an error if the socket cannot be opened or if the port is occupied, or if the socket is already set.
     *
     * @param hostName A const char* indicating the name of the host to whom to connect. "localhost" specifies that the host is on the same machine. Otherwise, use the name of the client.
     * @param portNum The number of the port on the host at which clients should connect.
     */
    void setSocket(const char* hostName, int portNum) {
        if (this->setUp)
            throw std::logic_error("Socket already set");
        
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
            throw std::runtime_error(strcat((char *)"ERROR getting host address: ", gai_strerror(returnVal))); //gai_strerror() returns a c string representation of the error
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
            throw std::runtime_error(strcat((char *)"ERROR opening socket: ", strerror(errno)));
        
        //No need to call bind() (see server side) because the local port number doesn't matter; the kernel will find an open port.
        
        /* connect()
         The connect() function is called by the client to establish a connection with the server, with three arguments.
         
         The first argument is the small integer reference for the socket.
         
         The second argument is the address of the host to connect to, including the port number, all in the form of a sockaddr struct.
         
         The third argument is the size of the address.
         
         The function returns 0 if successful and -1 if it fails.
         */
        if (connect(this->connectionSocket, serverAddress.ai_addr, serverAddress.ai_addrlen) < 0)
            throw std::runtime_error(strcat((char *)"ERROR connecting: ", strerror(errno)));
        
        freeaddrinfo(serverAddressList); //Free the linked list now that we have the local host information
        
        this->setUp = true; //All functions ensure the socket has been set before doing anything
    }
    
    /*!
     * A function that sends a message to the host. An error will be thrown if the socket is not set, if an error occurs in sending the message, or if the message is an empty string.
     *
     * @param message The message to be sent, as a const char*.
     * @param ensureFullStringSent An optional parameter that will make sure the full string is sent if it is too long to send with one call of write(). It is automatically set to false (so the rest of the string is not sent, but rather returned.
     *
     * @return Any part of the string that wasn't sent if the given string was too large to send in full. Only part of the string would have been sent, the rest is returned.
     */
    std::string send(const char* message, bool ensureFullStringSent = false) {
        if (!this->setUp)
            throw std::logic_error("Socket not set");
        
        //Empty messages won't be sent
        if (std::string(message) == "")
            throw std::logic_error("No message to send");
        
        unsigned long messageLength = strlen(message);
        
        long sentSize = write(this->connectionSocket, message, messageLength);
        
        if (sentSize < 0) {
            throw std::runtime_error(strcat((char *)"ERROR sending message: ", strerror(errno)));
        } else if (sentSize < messageLength) { //If only some of the string was sent, return what wasn't sent (or send the rest)
            std::string extraStr; //Holds the rest of the string that was not sent
            for (unsigned long a = sentSize; a < messageLength; a++) {
                extraStr += message[a];
            }
            if (ensureFullStringSent) { //This optional bool is set to false. If manually changed to true send the rest. Otherwise, return the unsent part
                this->send(extraStr.c_str(), ensureFullStringSent); //Recursively keep sending the rest of the string until it is all sent
                return "";
            }
            return extraStr; //Return any part of the string that was not sent. This occurs if the string is too long
        }
        return "";
    }
    
    /*!
     * A function that receives a message from the host. The function will wait for a short period for the client to send the message, and if the message is not received it will throw an error. An error is also thrown if the socket is not set.
     *
     * @param socketClosed An optional pointer to a bool that would be set to true if the client disconnected. Automatically set to a null pointer otherwise.
     *
     * @return The received message from the host.
     */
    std::string receive(bool* socketClosed = nullptr) {
        if (!this->setUp)
            throw std::logic_error("Socket not set");
        
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
            throw std::runtime_error(strcat((char *)"ERROR reading from socket: ", strerror(errno)));
        
        if (socketClosed != nullptr && messageSize == 0) {
            *socketClosed = true;
        }
        
        std::string str = std::string(buffer, messageSize);
        
        //Check if there is more data waiting to be read, and if so, read it
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(this->connectionSocket, &readfds);
        int n = this->connectionSocket + 1;
        
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 20000;
        
        int returnValue = select(n, &readfds, NULL, NULL, &timeout);
        if (returnValue < 0) {
            throw std::runtime_error(std::string("ERROR finding information about socket: ") + std::string(strerror(errno)));
        } else if (returnValue > 0) {
            str += this->receive();
        }
        
        return str;
    }
    
    /*!
     * A function to close the socket, so it can be rebound. Until it is set, other functions cannot be called.
     */
    void close() {
        if (!this->setUp)
            throw std::logic_error("Socket not set");
        
        ::close(this->connectionSocket);
        portNumber = 0;
        this->setUp = false;
    }
    
    /*!
     * A function to set a timeout for reading from the socket, until otherwise specified. If a socket times out and receive()'s optional bool pointer has been set, then it will indicate the socket closed. To reset to no timeout, set seconds to 0. A socket with a timeout will wait the given amount of time to receive a message, and stop listening after the alloted time has passed.
     *
     * @param seconds The number of seconds for which to set the timeout for. If 0, timeout will be cleared.
     * @param milliseconds An optional parameter indicating the number of milliseconds to add to the timeout. Autoinitialized as 0.
     */
    void setTimeout(unsigned int seconds, unsigned int milliseconds = 0) {
        if (!this->setUp)
            throw std::logic_error("Socket not set");
        
#if defined(_WIN32)
        DWORD timeout = (seconds * 1000) + milliseconds;
        setsockopt(this->hostSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
#else
        struct timeval time; //Time holds the maximum time to wait
        time.tv_sec = seconds;
        time.tv_usec = (milliseconds * 1000);
        
        /* setsockopt()
         The setsockopt() function changes options for a given socket, depending on the parameters.
         
         The first argument is the client side socket, passed by its file descriptor.
         
         The second argument is ...?
         
         The third argument tells what setting to change. SO_RCVTIMEO indicates the socket option is for the amount of time to wait while timing out.
         */
        setsockopt(this->connectionSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&time, sizeof(time));
#endif
    }
    
    /*!
     * @return If this object is set.
     */
    bool getSet() const {
        return this->setUp;
    }
    
private:
    //Private properties
    
    int connectionSocket; //This is the "file descriptor", which stores values from both the socket system call and the accept system call
    int portNumber; //The port nubmer where connections are accepted
    
    char buffer[BUFFER_SIZE];
    
    bool setUp = false; //Represents if the socket has already been set. If not, reading and writing will cause errors
};

#endif /* ClientSocket_hpp */
