//Standard library includes
#include <iostream>

//Local includes
#include "ServerSocket.hpp"
#include "ClientSocket.hpp"

int main(int argc, const char * argv[]) {
    /*
     This is a test which can be used to demonstrate either socket.
     It can be run as either the ServerSocket or ClientSocket, or by running two instances, as both.
     */
    std::cout << "Run as server? Type \'Y\' or \'y\' for yes, and anything else for no." << std::endl;
    char input;
    std::cin >> input;
    
    if (input == 'Y' || input == 'y') {
        ServerSocket server;
        server.setSocket(3000);
        server.addClient();
        server.send("Hello client!", 0);
        std::cout << "Server received " << server.receive(0);
    } else {
        ClientSocket client;
        client.setSocket("localhost", 3000);
        client.send("Hello server!");
        std::cout << "Client received " << client.receive();
    }
    return 0;
}
