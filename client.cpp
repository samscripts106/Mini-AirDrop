#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fstream>
using namespace std;

bool recvAll(int socket, char* data, long long totalBytes){
    long long totalReceived = 0;
    while(totalReceived<totalBytes){
        int bytesRecieved = recv(socket, data+totalReceived, totalBytes-totalReceived, 0);
        if(bytesRecieved<=0) return false;
        totalReceived += bytesRecieved;
    }
    return true; //totalReceived==totalBytes
}

int main(){
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    if(clientSocket == -1){
        cout<<"Socket creation failed\n";
        return 1;
    }

    //2. Specify server address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    inet_pton(AF_INET,"127.0.0.1",&serverAddress.sin_addr);

    //3. Connect to server
    if (connect(clientSocket,(struct sockaddr*)&serverAddress, sizeof(serverAddress))==-1){
        cout<<"Connection failed\n";
        return 1;
    }
    cout<<"Connected to server!\n";

    //4. Receive file size
    long long fileSize;
    // recv(clientSocket, &fileSize, sizeof(fileSize), 0); improvisation:
    if(!recvAll(clientSocket, reinterpret_cast<char*>(&fileSize), sizeof(fileSize))){
        cout<<"Failed to receive file size\n";
        return 1;
    }

    cout<<"File size: "<<fileSize<<" bytes\n";

    //5. Create destination file
    ofstream file ("received.txt", ios::binary);

    if(!file){
        cout<<"Could not create file\n";
        return 1;
    }

    //6. Receive file 
    char buffer[4096]; //Buffer where incoming network data temporarly goes
    long long totalReceived = 0; //How many bytes we received so far?

    //Keep receiving until we reach the end of the file
    while (totalReceived<fileSize){
        int bytesReceived  = recv(clientSocket,buffer,sizeof(buffer),0);
        if(bytesReceived <= 0) break; //In case something went wrong or connection closed

        file.write(buffer,bytesReceived); //Write the received bytes into our new file
        totalReceived += bytesReceived; //Keep track of how much file we received

        cout<< " \rReceived: "<< totalReceived <<" / "<<fileSize<<" bytes";
    }
    cout<<"\n File received!\n";

    file.close();
    return 0;
}