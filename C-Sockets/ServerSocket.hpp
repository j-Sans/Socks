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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <cerrno>

#define MAX_NUMBER_OF_CONNECTIONS 5
#define BUFFER_SIZE 65535

class ServerSocket {
public:
    //Constructor
    ServerSocket();
    
    //Destructor
    ~ServerSocket();
    
    //Static functions
    
    /*!
     * Returns the name of the host computer. Throws an error if the gethostname() from the standard C library fails.
     *
     * @return The name of the host.
     */
    static std::string getHostName();
    
    //Public member functions
    
    /*!
     * A function to initialize the socket. This must be done before the socket can be used. Will throw an error if the socket cannot be opened or if the port is occupied.
     *
     * @param portNum The number of the port on the host at which clients should connect.
     */
    void setSocket(int portNum);
    
    /*!
     * A function that adds a client. If there is no client, then the function waits for a connection to be initiated by the client. Will throw an error if the maximum number of sockets (see MAXIMUM_NUMBER_OF_SOCKETS) have already been set, or if an error occurs connecting to the client.
     */
    void addClient();
    
    /*!
     * A function that removes a client at a given index. If there is no client at that index, an error is thrown. An error will also be thrown if the socket has not been set.
     *
     * clientIndex The index of the client to close.
     */
    void closeConnection(unsigned int clientIndex);
    
    /*!
     * A function that sends a message to a single client. An error will be thrown if the socket is not set, if the given index is out of range, or if an error occurs in sending the message. If the optional parameter is set to true, an error will also be thrown if only part of the message was thrown.
     *
     * @param message The message to be sent, as a const char*.
     * @param clientIndex An unsigned int indicating the index of the client to whom to send the message.
     * @param throwErrorIfNotFullySent An optional boolean indicating if an error should be thrown if only part of the message was sent. Automatically set to false.
     */
    void send(const char* message, unsigned int clientIndex, bool throwErrorIfNotFullySent = false);
    
    /*!
     * A function that sends a message to all clients. An error will be thrown if the socket is not set or if an error occurs in sending the message to any of the clients. If the optional parameter is set to true, an error will also be thrown if only part of the message was thrown.
     *
     * @param message The message to be sent, as a const char*.
     * @param throwErrorIfNotFullySent An optional boolean indicating if an error should be thrown if only part of the message was sent. Automatically set to false.
     */
    void broadcast(const char* message, bool throwErrorIfNotFullySent = false);
    
    /*!
     * A function that receives a message from a single client. The function will wait for a short period for the client to send the message, and if the message is not received it will throw an error. An error is also thrown if the index is out of range or if the socket is not set.
     *
     * @param clientIndex An unsigned int indicating the index of the client from whom to receive the message.
     * @param socketClosed An optional pointer to a bool that would be set to true if the client disconnected. Automatically set to a null pointer otherwise.
     *
     * @return The received message from the specified client as a std::string.
     */
    std::string receive(unsigned int clientIndex, bool* socketClosed = nullptr);
    
    /*!
     * A function that checks if all clients sent a specific message. This function calls ServerSocket::receive() so if another message has been sent that message may be received instead. This function throws no errors other than those called by ServerSocket::receive() or ServerSocket::closeConnection(). Any sockets where connection was lost are automatically closed. 
     *
     * @param messageToCompare The message that is checked with all clients, as a const char*.
     *
     * @return True if all clients sent the same message as the one passed in. False otherwise.
     */
    bool allReceived(const char* messageToCompare);
    
    /*!
     * @return The number of clients of this socket.
     */
    unsigned int numberOfClients();
    
    /*!
     * @return If this object is set.
     */
    bool getSet();
    
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
    int hostSocket;
    
    bool activeConnections[MAX_NUMBER_OF_CONNECTIONS]; //Initialized as all false. True if the connection of that index is an active connection
    
    int clientSockets[MAX_NUMBER_OF_CONNECTIONS];
    
    /* struct sockaddr_storage {
        sa_family_t ss_family; //Either AF_INET or AF_INET6
        * A bunch of padding variables are also here. Ignore them. *
     }
     This struct is large enough that it can hold either an IPv4 or an IPv6 address (and be cast to either sockaddr_in or sockaddr_in6 if necessary)
     */
    sockaddr_storage clientAddresses[MAX_NUMBER_OF_CONNECTIONS];
    socklen_t clientAddressSizes[MAX_NUMBER_OF_CONNECTIONS];
    
    char buffer[BUFFER_SIZE];
    
    bool setUp = false; //Represents if the socket has already been set. If not, reading and writing will cause errors
    
    //Private member functions
    
    /*!
     * A function to get the next index to which a client can connect. -1 is returned if there are no more available indices.
     *
     * @return The next available index.
     */
    int getNextAvailableIndex();
    
};

#endif /* ServerSocket_hpp */
