#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace std;

int main() {
    //1. Server Socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0); //Requests OS to give an IPv4 TCP communication endpoint 

    if (serverSocket == -1) {
        cout << "Socket creation failed\n";
        return 1;
    }
    //2. Bind to port 8080
    sockaddr_in serverAddress; //A box containing IP Address, Port & Protocol information
    serverAddress.sin_family = AF_INET; //this address uses IPV4
    serverAddress.sin_addr.s_addr = INADDR_ANY; //Accept connections coming to any of this computer network's interfaces 
    serverAddress.sin_port = htons(8080);//Specifies port 8080

    if (::bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1){
        cout<<"Bind failed\n";
        return 1;
    } //binds the socket to a specific IP no. & port

    
    //3. Listen() - Wait for connection requests
    if (listen(serverSocket, 5) == -1){
        cout<<"Listen failed\n";
        return 1;
    }
    //puts the server into a state where its ready to receive connection requests
    //5 is the backlog (How many incoming connection requests can wait in the queue while our server is busy handling another connection)

    //4. Accept() - accept a client connection
    //5. Client Socket 
    int clientSocket = accept(serverSocket, nullptr, nullptr);

    if(clientSocket == -1){
        cout<<"Accept failed\n";
        return 1;
    }

    cout<<"Client connected\n";
    return 0;
}