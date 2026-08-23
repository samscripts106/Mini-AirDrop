#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QTimer>
#include <QFile>
#include <QByteArray>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fstream>
#include <fcntl.h>
#include <cstring>
#include <QDir>
#include <QDesktopServices>
#include <QUrl>
#include <QListWidgetItem>
#include <arpa/inet.h>
#include <QStringList>

void MainWindow::browseFile()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Select File"
        );

    if (!filePath.isEmpty()) {
        selectedFilePath = filePath;

        QFileInfo fileInfo(filePath);

        ui->fileLabel->setText(
            "Selected: " + fileInfo.fileName()
            );
    }
}

void MainWindow::sendFile()
{
    if (selectedFilePath.isEmpty()) {
        ui->fileLabel->setText("Please select a file first.");
        return;
    }

    if (selectedDeviceIp.isEmpty()) {
        ui->fileLabel->setText("Please select a device first.");
        return;
    }

    if (clientSocket == -1) {
        ui->fileLabel->setText("No phone connected.");
        return;
    }

    if (selectedDeviceIp != connectedDeviceIp) {
        ui->fileLabel->setText(
            "Selected device is not connected."
            );
        return;
    }

    bool success = sendSelectedFile();

    if (success) {
        ui->fileLabel->setText(
            "File sent successfully!"
            );
    }
}

bool MainWindow::sendAll(int socket, const char* data, long long totalBytes)
{
    long long totalSent = 0;

    while (totalSent < totalBytes) {

        ssize_t bytesSent = send(
            socket,
            data + totalSent,
            totalBytes - totalSent,
            0
            );

        if (bytesSent <= 0)
            return false;

        totalSent += bytesSent;
    }

    return true;
}

bool MainWindow::sendSelectedFile()
{
    if (selectedFilePath.isEmpty()) {
        ui->fileLabel->setText("No file selected.");
        return false;
    }

    std::ifstream file(
        selectedFilePath.toStdString(),
        std::ios::binary
        );

    if (!file) {
        ui->fileLabel->setText("Failed to open file.");
        return false;
    }

    file.seekg(0, std::ios::end);
    long long fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    QFileInfo fileInfo(selectedFilePath);
    QString fileName = fileInfo.fileName();

    QByteArray fileNameData = fileName.toUtf8();
    long long fileNameSize = fileNameData.size();

    if (!sendAll(
            clientSocket,
            reinterpret_cast<char*>(&fileNameSize),
            sizeof(fileNameSize)
            )) {
        ui->fileLabel->setText("Failed to send filename size.");
        return false;
    }

    if (!sendAll(
            clientSocket,
            fileNameData.data(),
            fileNameSize
            )) {
        ui->fileLabel->setText("Failed to send filename.");
        return false;
    }

    if (!sendAll(
            clientSocket,
            reinterpret_cast<char*>(&fileSize),
            sizeof(fileSize)
            )) {
        ui->fileLabel->setText("Failed to send file size.");
        return false;
    }

    const int bufferSize = 4096;
    char buffer[bufferSize];

    while (file.read(buffer, bufferSize) || file.gcount() > 0) {
        std::streamsize bytesRead = file.gcount();

        if (!sendAll(clientSocket, buffer, bytesRead)) {
            ui->fileLabel->setText("Failed while sending file.");
            return false;
        }
    }

    file.close();

    ui->fileLabel->setText("File sent successfully: " + fileName);

    return true;
}


void MainWindow::checkForClient()
{
    sockaddr_in clientAddress{};
    socklen_t clientLength = sizeof(clientAddress);

    int newSocket = accept(
        serverSocket,
        reinterpret_cast<sockaddr*>(&clientAddress),
        &clientLength
        );

    if (newSocket != -1) {

        if (clientSocket != -1)
            ::close(clientSocket);

        clientSocket = newSocket;

        char clientIp[INET_ADDRSTRLEN];

        inet_ntop(
            AF_INET,
            &clientAddress.sin_addr,
            clientIp,
            INET_ADDRSTRLEN
            );

        connectedDeviceIp =
            QString::fromUtf8(clientIp);

        outgoingConnection = false;

        qDebug() << "PHONE CONNECTED:"
                 << connectedDeviceIp;

        ui->fileLabel->setText(
            "Phone connected. Ready to receive."
            );
    }

    if (clientSocket != -1 &&
        !outgoingConnection) {
        receiveFile();
    }
}

void MainWindow::receiveFile()
{
    char buffer[4096];

    while (true) {

        int bytesReceived = recv(
            clientSocket,
            buffer,
            sizeof(buffer),
            0
            );

        if (bytesReceived == 0) {
            break;
        }

        if (bytesReceived < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;

            ::close(clientSocket);
            clientSocket = -1;
            return;
        }

        receiveBuffer.append(
            buffer,
            bytesReceived
            );
    }

    while (true) {

        if (expectedFileNameSize == -1) {

            if (receiveBuffer.size() <
                static_cast<int>(sizeof(long long)))
                return;

            memcpy(
                &expectedFileNameSize,
                receiveBuffer.constData(),
                sizeof(long long)
                );

            receiveBuffer.remove(
                0,
                sizeof(long long)
                );
        }

        if (receivedFileName.isEmpty()) {

            if (receiveBuffer.size() <
                expectedFileNameSize)
                return;

            receivedFileName =
                QString::fromUtf8(
                    receiveBuffer.constData(),
                    expectedFileNameSize
                    );

            receiveBuffer.remove(
                0,
                expectedFileNameSize
                );
        }

        if (expectedFileSize == -1) {

            if (receiveBuffer.size() <
                static_cast<int>(sizeof(long long)))
                return;

            memcpy(
                &expectedFileSize,
                receiveBuffer.constData(),
                sizeof(long long)
                );

            receiveBuffer.remove(
                0,
                sizeof(long long)
                );

            QString receivedFolder =
                "/Users/samscript06/MiniAirDrop/ReceivedFromPhone";

            QDir().mkpath(
                receivedFolder
                );

            QString savePath =
                receivedFolder +
                "/" +
                receivedFileName;

            receivedFile =
                new QFile(savePath);

            if (!receivedFile->open(
                    QIODevice::WriteOnly))
            {
                delete receivedFile;
                receivedFile = nullptr;

                receiveBuffer.clear();

                expectedFileNameSize = -1;
                expectedFileSize = -1;
                receivedFileName.clear();

                ui->fileLabel->setText(
                    "Failed to create received file."
                    );

                return;
            }
        }

        if (receivedFile &&
            !receiveBuffer.isEmpty()) {

            long long remaining =
                expectedFileSize -
                receivedFile->size();

            long long toWrite =
                qMin(
                    remaining,
                    static_cast<long long>(
                        receiveBuffer.size()
                        )
                    );

            receivedFile->write(
                receiveBuffer.constData(),
                toWrite
                );

            receiveBuffer.remove(
                0,
                toWrite
                );
        }

        if (receivedFile &&
            receivedFile->size() ==
                expectedFileSize) {

            receivedFile->close();

            delete receivedFile;
            receivedFile = nullptr;

            ui->fileLabel->setText(
                "Received: " +
                receivedFileName
                );

            expectedFileNameSize = -1;
            expectedFileSize = -1;
            receivedFileName.clear();

            return;
        }

        return;
    }
}

void MainWindow::openReceivedFolder()
{
    QString receivedFolder =
        "/Users/samscript06/MiniAirDrop/ReceivedFromPhone";

    QDir().mkpath(receivedFolder);

    QDesktopServices::openUrl(
        QUrl::fromLocalFile(receivedFolder)
        );
}

void MainWindow::processDiscoveryResponse()
{
    while (udpSocket->hasPendingDatagrams()) {

        QByteArray datagram;

        datagram.resize(
            static_cast<int>(
                udpSocket->pendingDatagramSize()
                )
            );

        QHostAddress senderAddress;
        quint16 senderPort;

        udpSocket->readDatagram(
            datagram.data(),
            datagram.size(),
            &senderAddress,
            &senderPort
            );

        QString response =
            QString::fromUtf8(datagram).trimmed();

        if (!discoveryActive)
            continue;

        QStringList parts =
            response.split("|");

        if (parts.size() < 3)
            continue;

        QString deviceName = parts[0];
        QString deviceMode = parts[1];
        QString phonePort = parts[2];

        QString ipAddress =
            senderAddress.toString();

        if (ipAddress.startsWith("::ffff:"))
            ipAddress = ipAddress.mid(7);

        QString deviceInfo =
            deviceName +
            " [" +
            deviceMode +
            "] (" +
            ipAddress +
            ")";

        if (ui->deviceList->findItems(
                              deviceInfo,
                              Qt::MatchExactly
                              ).isEmpty()) {

            QListWidgetItem *item =
                new QListWidgetItem(deviceInfo);

            item->setData(
                Qt::UserRole,
                ipAddress
                );

            item->setData(
                Qt::UserRole + 1,
                deviceMode
                );

            item->setData(
                Qt::UserRole + 2,
                phonePort
                );

            ui->deviceList->addItem(item);
        }
    }
}

void MainWindow::finishDiscovery()
{
    discoveryActive = false;

    if (ui->deviceList->count() == 0) {
        ui->fileLabel->setText(
            "No nearby devices found."
            );
    }
    else {
        ui->fileLabel->setText(
            QString::number(
                ui->deviceList->count()
                ) +
            " device(s) found."
            );
    }
}

void MainWindow::deviceSelected(QListWidgetItem *item)
{
    if (!item)
        return;

    selectedDeviceIp =
        item->data(Qt::UserRole).toString();

    QString deviceMode =
        item->data(Qt::UserRole + 1).toString();

    QString phonePort =
        item->data(Qt::UserRole + 2).toString();

    qDebug() << "Selected IP:" << selectedDeviceIp;
    qDebug() << "Device mode:" << deviceMode;
    qDebug() << "Phone port:" << phonePort;

    if (deviceMode != "RECEIVE") {

        ui->fileLabel->setText(
            "Selected: " +
            selectedDeviceIp +
            " [" +
            deviceMode +
            "]"
            );

        return;
    }

    int phonePortNumber =
        phonePort.toInt();

    int newSocket =
        socket(
            AF_INET,
            SOCK_STREAM,
            0
            );

    if (newSocket == -1) {

        ui->fileLabel->setText(
            "Failed to create connection."
            );

        return;
    }

    sockaddr_in phoneAddress{};

    phoneAddress.sin_family =
        AF_INET;

    phoneAddress.sin_port =
        htons(phonePortNumber);

    if (inet_pton(
            AF_INET,
            selectedDeviceIp.toStdString().c_str(),
            &phoneAddress.sin_addr
            ) <= 0) {

        ui->fileLabel->setText(
            "Invalid phone IP."
            );

        ::close(newSocket);
        return;
    }

    ui->fileLabel->setText(
        "Connecting to phone..."
        );

    if (::connect(
            newSocket,
            reinterpret_cast<sockaddr*>(&phoneAddress),
            sizeof(phoneAddress)
            ) == -1) {

        ui->fileLabel->setText(
            "Could not connect to phone."
            );

        qDebug()
            << "Connection failed:"
            << strerror(errno);

        ::close(newSocket);
        return;
    }

    if (clientSocket != -1)
        ::close(clientSocket);

    clientSocket = newSocket;

    connectedDeviceIp =
        selectedDeviceIp;

    outgoingConnection = true;

    ui->fileLabel->setText(
        "Phone connected. Ready to send."
        );

    qDebug()
        << "Connected to phone:"
        << selectedDeviceIp;
}

void MainWindow::discoverDevices()
{
    ui->deviceList->clear();
    ui->fileLabel->setText("Searching for nearby devices...");

    discoveryActive = true;

    QByteArray message =
        "MINI_AIRDROP_DISCOVERY";

    udpSocket->writeDatagram(
        message,
        QHostAddress::Broadcast,
        45454
        );

    discoveryTimer->start(2000);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    udpSocket = new QUdpSocket(this);

    udpSocket->bind(
        QHostAddress::AnyIPv4,
        45454,
        QUdpSocket::ShareAddress |
            QUdpSocket::ReuseAddressHint
        );

    discoveryTimer = new QTimer(this);
    discoveryTimer->setSingleShot(true);

    connect(ui->browseButton, &QPushButton::clicked,
            this, &MainWindow::browseFile);

    connect(ui->sendButton, &QPushButton::clicked,
            this, &MainWindow::sendFile);

    connect(ui->receivedButton, &QPushButton::clicked,
            this, &MainWindow::openReceivedFolder);

    connect(ui->discoverButton, &QPushButton::clicked,
            this, &MainWindow::discoverDevices);

    connect(udpSocket, &QUdpSocket::readyRead,
            this, &MainWindow::processDiscoveryResponse);

    connect(discoveryTimer, &QTimer::timeout,
            this, &MainWindow::finishDiscovery);

    connect(ui->deviceList, &QListWidget::itemClicked,
            this, &MainWindow::deviceSelected);

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        ui->fileLabel->setText("Failed to create server socket.");
        return;
    }
    fcntl(serverSocket, F_SETFL, O_NONBLOCK);

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(8080);

    if (::bind(
            serverSocket,
            (struct sockaddr*)&serverAddress,
            sizeof(serverAddress)
            ) == -1) {

        ui->fileLabel->setText("Bind failed.");
        return;
    }

    if (listen(serverSocket, 5) == -1) {
        ui->fileLabel->setText("Listen failed.");
        return;
    }

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout,
            this, &MainWindow::checkForClient);

    timer->start(100);
}

MainWindow::~MainWindow()
{
    delete ui;
}
