#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <cerrno>
#include <filesystem>
#include <thread>
#include <atomic>
#include <cstdio>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

namespace fs = std::filesystem;

std::atomic<int> phoneMode{0};

const int MAC_PORT = 8080;
const int PHONE_RECEIVE_PORT = 8081;

bool sendAll(int socket, const char* data, long long totalBytes)
{
    long long sent = 0;

    while (sent < totalBytes) {
        ssize_t bytes = send(
            socket,
            data + sent,
            totalBytes - sent,
            0
        );

        if (bytes <= 0)
            return false;

        sent += bytes;
    }

    return true;
}

bool receiveAll(int socket, char* data, long long totalBytes)
{
    long long received = 0;

    while (received < totalBytes) {
        ssize_t bytes = recv(
            socket,
            data + received,
            totalBytes - received,
            0
        );

        if (bytes <= 0)
            return false;

        received += bytes;
    }

    return true;
}

std::string getDeviceName()
{
    char buffer[256];
    std::string manufacturer;
    std::string model;

    FILE* pipe = popen(
        "getprop ro.product.manufacturer",
        "r"
    );

    if (pipe) {
        if (fgets(buffer, sizeof(buffer), pipe))
            manufacturer = buffer;

        pclose(pipe);
    }

    pipe = popen(
        "getprop ro.product.model",
        "r"
    );

    if (pipe) {
        if (fgets(buffer, sizeof(buffer), pipe))
            model = buffer;

        pclose(pipe);
    }

    while (!manufacturer.empty() &&
           (manufacturer.back() == '\n' ||
            manufacturer.back() == '\r')) {
        manufacturer.pop_back();
    }

    while (!model.empty() &&
           (model.back() == '\n' ||
            model.back() == '\r')) {
        model.pop_back();
    }

    if (manufacturer.empty() && model.empty())
        return "Android Device";

    if (manufacturer.empty())
        return model;

    if (model.empty())
        return manufacturer;

    return manufacturer + " " + model;
}

void discoveryListener()
{
    int udpSocket = socket(
        AF_INET,
        SOCK_DGRAM,
        0
    );

    if (udpSocket == -1)
        return;

    int reuse = 1;

    setsockopt(
        udpSocket,
        SOL_SOCKET,
        SO_REUSEADDR,
        &reuse,
        sizeof(reuse)
    );

    sockaddr_in address{};

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(45454);

    if (bind(
            udpSocket,
            reinterpret_cast<sockaddr*>(&address),
            sizeof(address)
        ) == -1) {

        close(udpSocket);
        return;
    }

    std::string deviceName = getDeviceName();

    while (true) {
        char buffer[1024];

        sockaddr_in senderAddress{};
        socklen_t senderLength =
            sizeof(senderAddress);

        ssize_t bytesReceived = recvfrom(
            udpSocket,
            buffer,
            sizeof(buffer) - 1,
            0,
            reinterpret_cast<sockaddr*>(&senderAddress),
            &senderLength
        );

        if (bytesReceived <= 0)
            continue;

        buffer[bytesReceived] = '\0';

        std::string message(buffer);

        if (message == "MINI_AIRDROP_DISCOVERY") {

            std::string mode;

            if (phoneMode == 1)
                mode = "SEND";
            else if (phoneMode == 2)
                mode = "RECEIVE";
            else
                mode = "NONE";

            std::string response =
                deviceName +
                "|" +
                mode +
                "|" +
                std::to_string(PHONE_RECEIVE_PORT);

            sendto(
                udpSocket,
                response.c_str(),
                response.size(),
                0,
                reinterpret_cast<sockaddr*>(&senderAddress),
                senderLength
            );
        }
    }

    close(udpSocket);
}

bool sendFile(int sock)
{
    std::string path;

    std::cout << "Enter file path: ";
    std::cin >> path;

    std::ifstream file(
        path,
        std::ios::binary
    );

    if (!file) {
        std::cout << "Failed to open file\n";
        return false;
    }

    file.seekg(0, std::ios::end);
    long long fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    size_t slash =
        path.find_last_of('/');

    std::string fileName;

    if (slash == std::string::npos)
        fileName = path;
    else
        fileName = path.substr(slash + 1);

    long long fileNameSize =
        fileName.size();

    if (!sendAll(
            sock,
            reinterpret_cast<char*>(&fileNameSize),
            sizeof(fileNameSize)
        ))
        return false;

    if (!sendAll(
            sock,
            fileName.data(),
            fileNameSize
        ))
        return false;

    if (!sendAll(
            sock,
            reinterpret_cast<char*>(&fileSize),
            sizeof(fileSize)
        ))
        return false;

    char buffer[4096];
    long long totalSent = 0;

    while (file) {

        file.read(
            buffer,
            sizeof(buffer)
        );

        std::streamsize bytesRead =
            file.gcount();

        if (bytesRead <= 0)
            break;

        if (!sendAll(
                sock,
                buffer,
                bytesRead
            ))
            return false;

        totalSent += bytesRead;

        std::cout
            << "\rSent: "
            << totalSent
            << " / "
            << fileSize
            << " bytes"
            << std::flush;
    }

    file.close();

    std::cout
        << "\nFile sent successfully: "
        << fileName
        << "\n";

    return true;
}

bool receiveFile(int sock)
{
    long long fileNameSize;

    if (!receiveAll(
            sock,
            reinterpret_cast<char*>(&fileNameSize),
            sizeof(fileNameSize)
        ))
        return false;

    if (fileNameSize <= 0 ||
        fileNameSize > 1024)
        return false;

    std::string fileName(
        fileNameSize,
        '\0'
    );

    if (!receiveAll(
            sock,
            fileName.data(),
            fileNameSize
        ))
        return false;

    long long fileSize;

    if (!receiveAll(
            sock,
            reinterpret_cast<char*>(&fileSize),
            sizeof(fileSize)
        ))
        return false;

    if (fileSize < 0)
        return false;

    std::string receiveFolder =
        "/data/data/com.termux/files/home/"
        "storage/shared/MiniAirDrop/"
        "ReceivedFromMac";

    try {
        fs::create_directories(
            receiveFolder
        );
    }
    catch (...) {
        return false;
    }

    fs::path safeFileName =
        fs::path(fileName).filename();

    std::string savePath =
        receiveFolder +
        "/" +
        safeFileName.string();

    std::ofstream file(
        savePath,
        std::ios::binary
    );

    if (!file) {
        std::cout
            << "Failed to create file\n";

        return false;
    }

    char buffer[4096];

    long long totalReceived = 0;

    while (totalReceived < fileSize) {

        long long remaining =
            fileSize -
            totalReceived;

        int chunkSize =
            remaining < sizeof(buffer)
                ? static_cast<int>(remaining)
                : sizeof(buffer);

        ssize_t bytes = recv(
            sock,
            buffer,
            chunkSize,
            0
        );

        if (bytes <= 0) {
            file.close();
            return false;
        }

        file.write(
            buffer,
            bytes
        );

        totalReceived += bytes;

        std::cout
            << "\rReceived: "
            << totalReceived
            << " / "
            << fileSize
            << " bytes"
            << std::flush;
    }

    file.close();

    std::cout
        << "\nReceived successfully: "
        << safeFileName.string()
        << "\n";

    std::cout
        << "Saved to: "
        << savePath
        << "\n";

    return true;
}

bool receiveFromMac()
{
    int serverSocket =
        socket(
            AF_INET,
            SOCK_STREAM,
            0
        );

    if (serverSocket == -1)
        return false;

    int reuse = 1;

    setsockopt(
        serverSocket,
        SOL_SOCKET,
        SO_REUSEADDR,
        &reuse,
        sizeof(reuse)
    );

    sockaddr_in address{};

    address.sin_family =
        AF_INET;

    address.sin_addr.s_addr =
        INADDR_ANY;

    address.sin_port =
        htons(PHONE_RECEIVE_PORT);

    if (bind(
            serverSocket,
            reinterpret_cast<sockaddr*>(&address),
            sizeof(address)
        ) == -1) {

        close(serverSocket);
        return false;
    }

    if (listen(
            serverSocket,
            1
        ) == -1) {

        close(serverSocket);
        return false;
    }

    std::cout
        << "\nWaiting for Mac...\n";

    sockaddr_in clientAddress{};

    socklen_t clientLength =
        sizeof(clientAddress);

    int clientSocket =
        accept(
            serverSocket,
            reinterpret_cast<sockaddr*>(&clientAddress),
            &clientLength
        );

    if (clientSocket == -1) {

        close(serverSocket);
        return false;
    }

    std::cout
        << "Mac connected!\n";

    bool success =
        receiveFile(clientSocket);

    close(clientSocket);
    close(serverSocket);

    return success;
}

bool sendToMac(
    const std::string& ip
)
{
    int sock =
        socket(
            AF_INET,
            SOCK_STREAM,
            0
        );

    if (sock == -1)
        return false;

    sockaddr_in serverAddress{};

    serverAddress.sin_family =
        AF_INET;

    serverAddress.sin_port =
        htons(MAC_PORT);

    if (inet_pton(
            AF_INET,
            ip.c_str(),
            &serverAddress.sin_addr
        ) <= 0) {

        close(sock);
        return false;
    }

    std::cout
        << "\nConnecting to "
        << ip
        << ":"
        << MAC_PORT
        << "...\n";

    if (connect(
            sock,
            reinterpret_cast<sockaddr*>(&serverAddress),
            sizeof(serverAddress)
        ) == -1) {

        std::cout
            << "Connection failed: "
            << strerror(errno)
            << "\n";

        close(sock);
        return false;
    }

    std::cout
        << "Connected to Mac!\n";

    bool success =
        sendFile(sock);

    close(sock);

    return success;
}

bool discoverMac(std::string& macIP)
{
    int udpSocket = socket(
        AF_INET,
        SOCK_DGRAM,
        0
    );

    if (udpSocket == -1)
        return false;

    int broadcast = 1;

    setsockopt(
        udpSocket,
        SOL_SOCKET,
        SO_BROADCAST,
        &broadcast,
        sizeof(broadcast)
    );

    timeval timeout{};
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;

    setsockopt(
        udpSocket,
        SOL_SOCKET,
        SO_RCVTIMEO,
        &timeout,
        sizeof(timeout)
    );

    sockaddr_in broadcastAddress{};

    broadcastAddress.sin_family =
        AF_INET;

    broadcastAddress.sin_port =
        htons(45454);

    broadcastAddress.sin_addr.s_addr =
        INADDR_BROADCAST;

    std::string message =
        "MINI_AIRDROP_DISCOVERY";

    sendto(
        udpSocket,
        message.c_str(),
        message.size(),
        0,
        reinterpret_cast<sockaddr*>(&broadcastAddress),
        sizeof(broadcastAddress)
    );

    std::cout
        << "\nSearching for devices...\n";

    char buffer[1024];

    sockaddr_in senderAddress{};
    socklen_t senderLength =
        sizeof(senderAddress);

    ssize_t bytesReceived =
        recvfrom(
            udpSocket,
            buffer,
            sizeof(buffer) - 1,
            0,
            reinterpret_cast<sockaddr*>(&senderAddress),
            &senderLength
        );

    if (bytesReceived <= 0) {

        std::cout
            << "No devices found.\n";

        close(udpSocket);
        return false;
    }

    buffer[bytesReceived] = '\0';

    std::string response(buffer);

    size_t firstSeparator =
        response.find('|');

    size_t secondSeparator =
        response.find(
            '|',
            firstSeparator + 1
        );

    if (firstSeparator == std::string::npos ||
        secondSeparator == std::string::npos) {

        close(udpSocket);
        return false;
    }

    std::string deviceName =
        response.substr(
            0,
            firstSeparator
        );

    std::string deviceMode =
        response.substr(
            firstSeparator + 1,
            secondSeparator -
            firstSeparator -
            1
        );

    if (deviceMode != "RECEIVE") {

        close(udpSocket);
        return false;
    }

    macIP =
        inet_ntoa(
            senderAddress.sin_addr
        );

    std::cout
        << "\nDevice found: "
        << deviceName
        << "\n";

    std::cout
        << "Connect? (y/n): ";

    char choice;
    std::cin >> choice;

    close(udpSocket);

    return choice == 'y' ||
           choice == 'Y';
}

int main()
{
    std::string deviceName =
        getDeviceName();

    std::cout
        << "============================\n"
        << "       Mini AirDrop\n"
        << "============================\n\n";

    std::cout
        << "Device: "
        << deviceName
        << "\n\n";

    int choice;

    std::cout
        << "1. Send File\n";

    std::cout
        << "2. Receive File\n";

    std::cout
        << "Choose: ";

    std::cin >> choice;

    if (choice != 1 &&
        choice != 2) {

        std::cout
            << "Invalid choice\n";

        return 1;
    }

    phoneMode = choice;

    std::thread discoveryThread(
        discoveryListener
    );

    discoveryThread.detach();

    if (choice == 1) {

        std::string macIP;

        if (!discoverMac(macIP))
            return 1;

        std::cout
            << "\nConnecting...\n";

        return sendToMac(macIP)
            ? 0
            : 1;
    }

    std::cout
        << "\nPhone is ready to receive.\n";

    std::cout
        << "Waiting for Mac connection...\n";

    return receiveFromMac()
        ? 0
        : 1;
}