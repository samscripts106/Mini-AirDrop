#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QByteArray>
#include <QFile>
#include <QString>
#include <QUdpSocket>
#include <QHostAddress>
#include <QList>
#include <QTimer>
#include <QListWidgetItem>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    Ui::MainWindow *ui;
    QString selectedFilePath;

    int serverSocket = -1;
    int clientSocket = -1;

    bool sendAll(int socket, const char* data, long long totalBytes);
    bool sendSelectedFile();

    void receiveFile();

    QByteArray receiveBuffer;
    long long expectedFileNameSize = -1;
    long long expectedFileSize = -1;
    QString receivedFileName;
    QFile *receivedFile = nullptr;

    QUdpSocket *udpSocket;
    QTimer *discoveryTimer;
    bool discoveryActive = false;
    QString selectedDeviceIp;
    QString connectedDeviceIp;
    bool outgoingConnection = false;

    void discoverDevices();
    void processDiscoveryResponse();
    void finishDiscovery();

private slots:
    void browseFile();
    void sendFile();
    void checkForClient();
    void openReceivedFolder();
    void deviceSelected(QListWidgetItem *item);
};

#endif