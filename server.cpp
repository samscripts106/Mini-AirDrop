#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace std;

int main() {

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0); //Requests OS to give an IPv4 TCP communication endpoint 

    if (serverSocket == -1) {
        cout << "Socket creation failed\n";
        return 1;
    }

    sockaddr_in serverAddress; //A box containing IP Address, Port & Protocol information
    serverAddress.sin_family = AF_INET; //this address uses IPV4
    serverAddress.sin_addr.s_addr = INADDR_ANY; //Accept connections coming to any of this computer network's interfaces 
    serverAddress.sin_port = htons(8080);//Specifies port 8080

    if (::bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1){
        cout<<"Bind failed\n";
        return 1;
    } //binds the socket to a specific IP no. & port

    cout << "Server bound to port 8080\n";

    return 0;
}