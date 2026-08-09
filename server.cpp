#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

int main() {

    // 1. Create socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocket == -1) {
        std::cerr << "Socket creation failed\n";
        return 1;
    }

    std::cout << "Socket created!\n";


    // 2. Create server address
    sockaddr_in serverAddress{};

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;


    // 3. Bind socket to port
    if (bind(serverSocket,
             (sockaddr*)&serverAddress,
             sizeof(serverAddress)) == -1) {

        std::cerr << "Bind failed\n";
        close(serverSocket);
        return 1;
    }

    std::cout << "Bind successful!\n";


    // 4. Start listening
    if (listen(serverSocket, 5) == -1) {
        std::cerr << "Listen failed\n";
        close(serverSocket);
        return 1;
    }

    std::cout << "Server is listening on port 8080...\n";


    // Keep socket open for now
    while (true) {
    }


    close(serverSocket);

    return 0;
}