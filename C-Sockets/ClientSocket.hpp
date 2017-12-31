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
    ClientSocket(const char* hostName, int portNum);
    
    //Destructor
    ~ClientSocket();
    
    //Public member functions
    
    /*!
     * A function to initialize the socket. This must be done before the socket can be used. Will throw an error if the socket cannot be opened or if the port is occupied, or if the socket is already set.
     *
     * @param hostName A const char* indicating the name of the host to whom to connect. "localhost" specifies that the host is on the same machine. Otherwise, use the name of the client.
     * @param portNum The number of the port on the host at which clients should connect.
     */
    void setSocket(const char* hostName, int portNum);
    
    /*!
     * A function that sends a message to the host. An error will be thrown if the socket is not set, if an error occurs in sending the message, or if the message is an empty string.
     *
     * @param message The message to be sent, as a const char*.
     * @param ensureFullStringSent An optional parameter that will make sure the full string is sent if it is too long to send with one call of write(). It is automatically set to false (so the rest of the string is not sent, but rather returned.
     *
     * @return Any part of the string that wasn't sent if the given string was too large to send in full. Only part of the string would have been sent, the rest is returned.
     */
    std::string send(const char* message, bool ensureFullStringSent = false);
    
    /*!
     * A function that receives a message from the host. The function will wait for a short period for the client to send the message, and if the message is not received it will throw an error. An error is also thrown if the socket is not set.
     *
     * @param socketClosed An optional pointer to a bool that would be set to true if the client disconnected. Automatically set to a null pointer otherwise.
     *
     * @return The received message from the host.
     */
    std::string receive(bool* socketClosed = nullptr);
    
    /*!
     * A function to close the socket, so it can be rebound. Until it is set, other functions cannot be called.
     */
    void close();
    
    /*!
     * A function to set a timeout for reading from the socket, until otherwise specified. If a socket times out and receive()'s optional bool pointer has been set, then it will indicate the socket closed. To reset to no timeout, set seconds to 0. A socket with a timeout will wait the given amount of time to receive a message, and stop listening after the alloted time has passed.
     *
     * @param seconds The number of seconds for which to set the timeout for. If 0, timeout will be cleared.
     * @param milliseconds An optional parameter indicating the number of milliseconds to add to the timeout. Autoinitialized as 0.
     */
    void setTimeout(unsigned int seconds, unsigned int milliseconds = 0);
    
    /*!
     * @return If this object is set.
     */
    bool getSet() const;
    
private:
    //Private properties
    
    int connectionSocket; //This is the "file descriptor", which stores values from both the socket system call and the accept system call
    int portNumber; //The port nubmer where connections are accepted
    
    char buffer[BUFFER_SIZE];
    
    bool setUp = false; //Represents if the socket has already been set. If not, reading and writing will cause errors
};

#endif /* ClientSocket_hpp */
