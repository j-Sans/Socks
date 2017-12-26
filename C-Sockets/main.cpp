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
    
    char input;
    
    if (argc > 1) {
        if (argv[2][0] == 'y' || argv[2][0] == 'Y') input = 'y';
        else if (argv[2][0] == 'n' || argv[2][0] == 'N') input = 'n';
        else {
            std::cout << "Run as server? Type \'Y\' or \'y\' for yes, and anything else for no." << std::endl;
            std::cin >> input;
        }
    } else {
        std::cout << "Run as server? Type \'Y\' or \'y\' for yes, and anything else for no." << std::endl;
        std::cin >> input;
    }
    
    if (input == 'Y' || input == 'y') {
        ServerSocket server;
        server.setSocket(3000, 1);
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
