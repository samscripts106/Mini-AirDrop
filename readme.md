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

### 📤 Send from Android → Mac

1. Start the MiniAirDrop application on the Mac.
2. Open the MiniAirDrop client in Termux on Android.
3. Enter the Mac's IP address when prompted.
4. Select **Send File** from the Android client.
5. Enter the path of the file you want to send.
6. The Android device connects to the Mac over TCP.
7. The selected file is transferred and saved on the Mac.
   
   <img width="1080" height="940" alt="image" src="https://github.com/user-attachments/assets/8fa5f3bf-2a4c-4f36-bf31-269cfbea5e37" />


### 📥 Receive on Android from Mac

1. Open the MiniAirDrop client in Termux.
2. Select **Receive File** to start the Android receiving service.
3. Open MiniAirDrop on the Mac.
4. Use **Device Discovery** to find the Android device on the local network.
5. Select the discovered Android device.
6. Choose a file on the Mac and click **Send**.
7. The file is transferred directly to the Android device.
   
<img width="1080" height="949" alt="image" src="https://github.com/user-attachments/assets/12edde92-abcf-4205-a8df-e33085b88dcb" />
<img width="1600" height="1125" alt="image" src="https://github.com/user-attachments/assets/56085743-f82e-4088-b0d7-7a5f2fee91e3" />

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

## ⚠️ Current limitations:

The current version uses a **Termux-based client on Android**, so there isn't a native mobile GUI yet. The project is also currently designed and tested for **macOS ↔ Android transfers**, with laptop-to-laptop support still being a future improvement.

