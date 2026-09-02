# MiniAirDrop 🚀

A lightweight **cross-device file transfer application** built with **C++** that allows files to be transferred between a **Mac and an Android device** over the same local network.

The project was inspired by Apple's AirDrop and focuses on understanding the fundamentals of **computer networking, socket programming, and file transfer protocols**.

## ✨ Features

* 📤 Send files between Mac and Android
* 📥 Receive files on both devices
* 🔍 Automatic device discovery using UDP
* 🔌 Reliable file transfer using TCP
* 🖥️ Simple desktop GUI built with Qt
* 📱 Android support through Termux

## 🏗️ Architecture

MiniAirDrop uses two networking protocols:

### UDP — Device Discovery

UDP broadcasts are used to discover devices available on the local network.

```text
Mac  ───── UDP Broadcast ─────► Android
Mac  ◄──── Device Response ──── Android
```

The discovery service uses port:

```text
45454
```

### TCP — File Transfer

Once a device is selected, files are transferred using TCP for reliable delivery.

```text
Sender
   │
   │  Filename Size
   │  Filename
   │  File Size
   │  File Data
   ▼
Receiver
```

Ports used:

| Device  |  Port | Purpose              |
| ------- | ----: | -------------------- |
| Mac     |  8080 | File receiving       |
| Android |  8081 | File receiving       |
| Both    | 45454 | UDP device discovery |

## 🛠️ Tech Stack

* **C++**
* **Qt 6**
* **QTcpSocket / QTcpServer**
* **QUdpSocket**
* **POSIX Socket Programming**
* **Termux**
* **CMake**

## 📱 Supported Platforms

Currently tested between:

* macOS 🍎 (using Qt GUI)
* Android 📱 (using Termux)

Both devices must be connected to the **same local network or hotspot**.

## 🚀 How It Works

1. Open MiniAirDrop on the Mac.
2. Discover available devices on the local network.
3. Select a device.
4. Choose a file.
5. Click **Send**.
6. The file is transferred directly between the devices using TCP.

No cloud storage or third-party server is required.

## 🧠 Networking Concepts Used

This project helped explore:

* TCP/IP networking
* Client-server architecture
* Socket programming
* UDP broadcasting
* Device discovery
* File transfer protocols
* Byte streams and packet handling
* Reliable data transmission
* Local network communication
