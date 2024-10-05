# 🖐️ ULAttendance: Fingerprint Attendance System

## 🏫 A Project from Universidad de Los Andes (ULA), Venezuela

### 👨‍🔬 Developed by Omar Andrade and Eduardo Sulbaran

---

## 🌟 Project Overview

Welcome to ULAttendance, an innovative fingerprint-based attendance system born in the heart of the Andes! This project combines cutting-edge biometric technology with the spirit of Venezuelan ingenuity to revolutionize how attendance is tracked in educational and professional settings.

### 🇻🇪 Made in Venezuela, for the World

ULAttendance is proudly developed at the Universidad de Los Andes, showcasing the talent and creativity of Venezuelan engineers. Our system proves that great innovations can come from anywhere, even amidst challenges.

---

## 🔑 Key Features

- **Biometric Authentication**: Utilizes AS608 fingerprint sensor for secure and quick identification.
- **User-Friendly Interface**: TFT display and keypad for easy interaction and setup.
- **Robust Data Management**: Efficient storage and retrieval of user data and attendance logs.
- **Time Synchronization**: Keeps accurate time with SNTP, ensuring precise attendance records.
- **Flexible Connectivity**: WiFi capabilities for potential future expansions.
- **Scalable Design**: Can handle multiple users and extensive attendance logs.

---

## 🛠️ Technology Stack

- **Hardware**: ESP32 microcontroller, AS608 fingerprint sensor, TFT display, Keypad
- **Software**: FreeRTOS, ESP-IDF framework
- **Storage**: NVS (Non-Volatile Storage), SD Card support
- **Connectivity**: WiFi for time synchronization and potential future features

---

## 🚀 Getting Started

1. **Clone the Repository**
   ```
   git clone https://github.com/ULA-Venezuela/ULAttendance.git
   ```

2. **Set Up ESP-IDF**
   Follow the [official ESP-IDF setup guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)

3. **Configure the Project**
   ```
   idf.py menuconfig
   ```
   Set your WiFi SSID and password in the project configuration.

4. **Build and Flash**
   ```
   idf.py build
   idf.py -p (PORT) flash
   ```

5. **Monitor the Output**
   ```
   idf.py monitor
   ```

---

## 🤝 Contributing

We welcome contributions from the global community! Whether you're fixing bugs, improving documentation, or proposing new features, your efforts are appreciated.

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
3. Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the Branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

---

## 📜 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

## 🙏 Acknowledgments

- Universidad de Los Andes for fostering innovation
- The open-source community for invaluable tools and libraries
- Our families and friends for their unwavering support

---

**¡Viva la innovación venezolana!** 🇻🇪✨
