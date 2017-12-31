//
//  ServerSocket.hpp
//  Server_C_Socket
//
//  Created by Jake Sanders on 9/5/16.
//  Copyright © 2016 Jake Sanders. All rights reserved.
//

#ifndef ServerSocket_hpp
#define ServerSocket_hpp

#include <string>
#include <vector>
#include <exception>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <cerrno>

#define BUFFER_SIZE 65535

class ServerSocket {
public:
    //Constructor
    ServerSocket() {}
    ServerSocket(int portNum, int maxConnections) {
        this->setSocket(portNum, maxConnections);
    }
    
    //Destructor
    ~ServerSocket() {
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
    
    //Static functions
    
    /*!
     * Returns the name of the host computer. Throws an error if the gethostname() from the standard C library fails.
     *
     * @return The name of the host.
     */
    static std::string getHostName() {
        char name[1024];
        if (gethostname(name, 1024) < 0) { //Fill name with the host name. Then, if an error occurred, throw an exception
            throw std::runtime_error(strcat((char *)"ERROR getting host name: ", strerror(errno)));
        }
        return std::string(name);
    }
    
    //Public member functions
    
    /*!
     * A function to initialize the socket. This must be done before the socket can be used. Will throw an error if the socket cannot be opened or if the port is occupied, or if the socket is already set.
     *
     * @param portNum The number of the port on the host at which clients should connect.
     * @param maxConnections The max number of clients that this host can theoretically connect with.
     */
    void setSocket(int portNum, int maxConnections) {
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
    
    /*!
     * A function that adds a client. If there is no client, then the function waits for a connection to be initiated by the client. Will throw an error if the maximum number of sockets (see MAXIMUM_NUMBER_OF_SOCKETS) have already been set, or if an error occurs connecting to the client.
     */
    void addClient() {
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
    
    /*!
     * A function that removes a client at a given index. If there is no client at that index, an error is thrown. An error will also be thrown if the socket has not been set.
     *
     * clientIndex The index of the client to close.
     */
    void closeConnection(unsigned int clientIndex) {
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
    
    /*!
     * A function that sends a message to a single client. An error will be thrown if the socket is not set, if the given index is out of range, if an error occurs in sending the message, or if the message is an empty string.
     *
     * @param message The message to be sent, as a const char*.
     * @param clientIndex An unsigned int indicating the index of the client to whom to send the message.
     * @param ensureFullStringSent An optional parameter that will make sure the full string is sent if it is too long to send with one call of write(). It is automatically set to false (so the rest of the string is not sent, but rather returned.
     *
     * @return Any part of the string that wasn't sent if the given string was too large to send in full. Only part of the string would have been sent, the rest is returned.
     */
    std::string send(const char* message, unsigned int clientIndex, bool ensureFullStringSent = false) {
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
    
    /*!
     * A function that sends a message to all clients. An error will be thrown if the socket is not set or if an error occurs in sending the message to any of the clients. If the optional parameter is set to true, an error will also be thrown if only part of the message was thrown.
     *
     * @param message The message to be sent, as a const char*.
     * @param ensureFullStringSent An optional parameter that will make sure the full string is sent if it is too long to send with one call of write(). It is automatically set to false (so the rest of the string is not sent, but rather discarded.
     */
    void broadcast(const char* message, bool ensureFullStringSent = false) {
        if (!this->setUp)
            throw std::logic_error("Socket not set");
        
        //Send the message to each active client
        for (int a = 0; a < this->activeConnections.size(); a++) {
            if (this->activeConnections[a]) {
                this->send(message, a, ensureFullStringSent);
            }
        }
    }
    
    /*!
     * A function that receives a message from a single client. The function will wait for a short period for the client to send the message, and if the message is not received it will throw an error. An error is also thrown if the index is out of range or if the socket is not set.
     *
     * @param clientIndex An unsigned int indicating the index of the client from whom to receive the message.
     * @param socketClosed An optional pointer to a bool that would be set to true if the client disconnected. Automatically set to a null pointer otherwise.
     *
     * @return The received message from the specified client as a std::string.
     */
    std::string receive(unsigned int clientIndex, bool* socketClosed = nullptr) {
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
    
    /*!
     * A function that checks if all clients sent a specific message. This function calls ServerSocket::receive() so if another message has been sent that message may be received instead, and thus will not be read or returned by the server. This function throws no errors other than those called by ServerSocket::receive() or ServerSocket::closeConnection(). Any sockets where connection was lost are automatically closed.
     *
     * @param messageToCompare The message that is checked with all clients, as a const char*.
     *
     * @return True if all clients sent the same message as the one passed in. False otherwise.
     */
    bool receivedFromAll(const char* messageToCompare) {
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
    
    /*!
     * A function to set a timeout for reading from the socket, until otherwise specified. If a socket times out and receive()'s optional bool pointer has been into it, then it will indicate the socket closed. To reset to no timeout, set seconds to 0. Only set for the connections with other clients.
     *
     * @param seconds The number of seconds for which to set the timeout for. If 0, timeout will be cleared.
     * @param milliseconds An optional parameter indicating the number of milliseconds to add to the timeout. Autoinitialized as 0.
     */
    void setTimeout(unsigned int seconds, unsigned int milliseconds = 0) {
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
    
    /*!
     * A function to set a timeout for host socket actions, like listening for other sockets and accepting them. To reset to no timeout, set seconds to 0.
     *
     * @param seconds The number of seconds for which to set the timeout for. If 0, timeout will be cleared.
     * @param milliseconds An optional parameter indicating the number of milliseconds to add to the timeout. Autoinitialized as 0.
     */
    void setHostTimeout(unsigned int seconds, unsigned int milliseconds = 0) {
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
    
    /*!
     * @return The number of clients of this socket.
     */
    unsigned int numberOfClients() const {
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
    
    /*!
     * @return If this object is set.
     */
    bool isSet() const {
        return this->setUp;
    }
    
private:
    //Private properties
    
    int portNumber; //The port nubmer where connections are accepted
    
    /* struct addrinfo {
         int ai_flags; //AI_PASSIVE, AI_CANONNAME, etc...
         int ai_family; //AF_INET, AF_INET6, AF_UNSPEC. This is the type of IP address (IPv4, IPv6, or any, respectively)
         int ai_socktype; //SOCK_STREAM, SOCK_DGRAM (TCP and UDP, respectively)
         int ai_protocol; //0 represents any
         size_t ai_addrlen; //Represents the size of the following property ai_addr, in bytes
         struct sockaddr* ai_addr; //A struct containing the address info. Can be a sockaddr_in or sockaddr_in6
         char* hostname; //Full hostname
         
         struct addrinfo* ai_next; //The next addrinfo node. Used when addrinfo structs are returned as a linked list
     }
     */
    addrinfo serverAddress;
    
    //These are "file descriptors", which store values from both the socket system call and the accept system call
    int hostSocketFD;
    
    std::vector<bool> activeConnections;//[MAX_NUMBER_OF_CONNECTIONS]; //Initialized as all false. True if the connection of that index is an active connection
    
    std::vector<int> clientSocketsFD;//[MAX_NUMBER_OF_CONNECTIONS];
    
    /* struct sockaddr_storage {
        sa_family_t ss_family; //Either AF_INET or AF_INET6
        * A bunch of padding variables are also here. Ignore them. *
     }
     This struct is large enough that it can hold either an IPv4 or an IPv6 address (and be cast to either sockaddr_in or sockaddr_in6 if necessary)
     */
    std::vector<sockaddr_storage> clientAddresses;//[MAX_NUMBER_OF_CONNECTIONS];
    std::vector<socklen_t> clientAddressSizes;//[MAX_NUMBER_OF_CONNECTIONS];
    
    char buffer[BUFFER_SIZE];
    
    bool setUp = false; //Represents if the socket has already been set. If not, reading and writing will cause errors
    
    //Private member functions
    
    /*!
     * A function to get the next index to which a client can connect. -1 is returned if there are no more available indices.
     *
     * @return The next available index.
     */
    int getNextAvailableIndex() const {
        if (!this->setUp)
            throw std::logic_error("Socket not set");
        
        for (int a = 0; a < this->activeConnections.size(); a++) {
            if (!this->activeConnections[a]) return a;
        }
        return -1;
    }
    
    
};

#endif /* ServerSocket_hpp */
