#include <iostream>
#include <sys/socket.h>

using namespace std;

int main() {

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocket == -1) {
        cout << "Socket creation failed\n";
        return 1;
    }

    cout << "Socket created successfully!\n";

    return 0;
}