//Standard library includes
#include <iostream>

//Local includes
#include "ServerSocket.hpp"
#include "ClientSocket.hpp"

int main(int argc, const char * argv[]) {
    int a;
    
    std::cin >> a;
    
    if (a == 0) {
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
