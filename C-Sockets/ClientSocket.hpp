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
    ClientSocket();
    
    //Destructor
    ~ClientSocket();
    
    //Public member functions
    
    /*!
     * A function to initialize the socket. This must be done before the socket can be used. Will throw an error if the socket cannot be opened or if the port is occupied.
     *
     * @param hostName A const char* indicating the name of the host to whom to connect. "localhost" specifies that the host is on the same machine. Otherwise, use the name of the client.
     * @param portNum The number of the port on the host at which clients should connect.
     */
    void setSocket(const char* hostName, int portNum);
    
    /*!
     * A function that sends a message to the host. An error will be thrown if the socket is not set or if an error occurs in sending the message.
     *
     * @param message A std::string of the message to be sent.
     * @param throwErrorIfNotFullySent An optional boolean indicating if an error should be thrown if only part of the message was sent. Automatically set to false.
     */
    void send(const char* message, bool throwErrorIfNotFullySent = false);
    
    /*!
     * A function that receives a message from the host. The function will wait for a short period for the client to send the message, and if the message is not received it will throw an error. An error is also thrown if the socket is not set.
     *
     * @param socketClosed An optional pointer to a bool that would be set to true if the client disconnected. Automatically set to a null pointer otherwise.
     *
     * @return The received message from the host.
     */
    std::string receive(bool* socketClosed = nullptr);
    
    /*!
     * @return If this object is set.
     */
    bool getSet();
    
private:
    //Private properties
    
    int connectionSocket; //This is the "file descriptor", which stores values from both the socket system call and the accept system call
    int portNumber; //The port nubmer where connections are accepted
    
    bool setUp = false; //Represents if the socket has already been set. If not, reading and writing will cause errors
};

#endif /* ClientSocket_hpp */
