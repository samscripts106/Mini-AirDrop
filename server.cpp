#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fstream>
using namespace std;

bool sendAll(int socket, const char* data, long long totalBytes){
    long long totalSent = 0;
    while(totalSent<totalBytes){
        int bytesSent = send(socket, data+totalSent, totalBytes-totalSent, 0); 
        if(bytesSent<=0) return false; 
        totalSent += bytesSent;
    }
    return true; //totalSent==totalBytes
}

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
    cout<<"Server is listening on port 8080...\n";

    //puts the server into a state where its ready to receive connection requests
    //5 is the backlog (How many incoming connection requests can wait in the queue while our server is busy handling another connection)

    //4. Accept() - accept a client connection
    int clientSocket = accept(serverSocket, nullptr, nullptr);

    if(clientSocket == -1){
        cout<<"Accept failed\n";
        return 1;
    }
    cout<<"Client connected\n";

    //5. Open file
    ifstream file ("test.txt", ios::binary);
    if(!file){
        cout<<"Could not open file\n";
        return 1;
    }

    //6. Find file size
    file.seekg(0, ios::end); 
    long long fileSize = file.tellg(); 
    file.seekg(0, ios::beg);

    //7. Send the file size
    // send(clientSocket, &fileSize, sizeof(fileSize), 0); improvisation:
    if(!sendAll(clientSocket, reinterpret_cast<char*> (&fileSize), sizeof(fileSize))){
        cout<<"Failed to send the file size\n";
        return 1;
    }

    //8. Send file
    char buffer[4096]; //Temporrary memory area (We will read a small part of file into this)

    //Keep reading the file until we reach the end
    while(file){
        file.read(buffer,sizeof(buffer)); //Read upto 4096 bytes from the file & put it into buffer
        streamsize bytesRead = file.gcount(); //How many bytes did we actually read (usually 4096 but the file chunk may be smaller)

        //Only send if we actually read something
        if(bytesRead>0){
            // send(clientSocket, buffer, bytesRead, 0); improvisation:
            if(!sendAll(clientSocket, buffer, bytesRead)){
                cout<<"Failed to send file\n";
                return 1;
            }
        }
    }

    cout<<"File sent succesfully!\n";

    file.close();
    return 0;
}