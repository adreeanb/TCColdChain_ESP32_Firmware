# TCColdChain_ESP32_Firmware

A full-stack IoT solution designed to monitor temperature-sensitive environments in real-time. Built with a Store-and-Forward architecture to ensure zero data loss during network outages.
## 🏗️ System Architecture* 
**Hardware:** ESP32 microcontroller with a DS18B20 temperature sensor.* 
**Communication:** MQTT protocol for lightweight telemetry.* 
🚀 Key Features* **Store-and-Forward:** The ESP32 buffers temperature data locally when offline and flushes the queue sequentially upon reconnection.* 
**Real-Time Telemetry:** Live temperature updates pushed to the frontend.* 
**Data Persistence:** Reliable historical logging for compliance and analysis.## 

💻 Local Setup### 1. Firmware (ESP32)
Navigate to the `firmware/` directory and open the project in PlatformIO or Arduino IDE.
Rename `include/config.example.h` to `include/config.h` and add your Wi-Fi and MQTT broker credentials. 
