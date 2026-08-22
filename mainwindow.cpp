#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QTimer>
#include <QFile>
#include <QDir>
#include <QByteArray>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fstream>
#include <fcntl.h>
#include <cstring>
#include <QDir>

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

    if (clientSocket == -1) {
        ui->fileLabel->setText("No client connected.");
        return;
    }

    bool success = sendSelectedFile();

    if (success) {
        ui->fileLabel->setText("File sent successfully!");
    }
}

bool MainWindow::sendAll(int socket, const char* data, long long totalBytes)
{
    long long totalSent = 0;

    while (totalSent < totalBytes) {
        int bytesSent = send(
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
    if (clientSocket == -1) {
        sockaddr_in clientAddress{};
        socklen_t clientLength = sizeof(clientAddress);

        int newSocket = accept(
            serverSocket,
            (struct sockaddr*)&clientAddress,
            &clientLength
            );

        if (newSocket != -1) {
            clientSocket = newSocket;
            fcntl(clientSocket, F_SETFL, O_NONBLOCK);

            ui->fileLabel->setText(
                "Client connected."
                );
        }

        return;
    }

    receiveFile();
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
            ::close(clientSocket);
            clientSocket = -1;
            return;
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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->browseButton, &QPushButton::clicked,
            this, &MainWindow::browseFile);

    connect(ui->sendButton, &QPushButton::clicked,
            this, &MainWindow::sendFile);

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
