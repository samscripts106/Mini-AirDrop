#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

int main(){
    int clinetSocket = socket(AF_INET, SOCK_STREAM, 0);

    if(clinetSocket == -1){
        cout<<"Socket creation failed\n";
        return 1;
    }

    //2. Specify server address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    inet_pton(AF_INET,"127.0.0.1",&serverAddress.sin_addr);

    //3. Connect to server
    if (connect(clinetSocket,(struct sockaddr*)&serverAddress, sizeof(serverAddress))==-1){
        cout<<"Connection failed\n";
        return 1;
    }
    cout<<"Connected to server!\n";
    return 0;
}