# 🌿 ESP32-Based Air Quality Monitoring Station HOME 🌿

[![Codacy Badge](https://api.codacy.com/project/badge/Grade/f300c2f36eb94763bba094d57e5c5512)](https://app.codacy.com/gh/CyberScopeToday/ESP32-AirQuality-Station-Home?utm_source=github.com&utm_medium=referral&utm_content=CyberScopeToday/ESP32-AirQuality-Station-Home&utm_campaign=Badge_Grade)

<img src="https://github.com/user-attachments/assets/b73504b6-6a63-4535-9e27-ee8864384fa2" width="50%">


> **An IoT project for monitoring CO₂ levels, temperature, humidity, and air quality with a user-friendly interface, RGB indication, and control via a Telegram bot.** 🎉

---

**[🇷🇺 Click here for the Russian version of the README (Нажмите здесь для русской версии README)](readmeru.md)**

---

## 📋 Description

This air quality monitoring station measures:

- 🌡️ **Temperature** (sensor **BME680**)
- 💧 **Humidity** (sensor **BME680**)
- 🌫️ **Air Quality** (CO₂ concentration via **MH-Z19B** and VOC via **BME680**)

### 📊 Acquired Data:

- **Displayed** on an OLED SSD1306 display 📺
- **Indicated** using an RGB LED 💡
- **Sent** to the **ThingSpeak** platform 🌐
- **Accessible** via a **Telegram bot** 🤖

---

## 🛠 Used Technologies

| Technology            | Description                                                      |
|-----------------------|------------------------------------------------------------------|
| ESP32                 | Main controller of the project                                   |
| ![Arduino IDE](https://img.icons8.com/color/48/000000/arduino.png) **Arduino IDE** | Development environment for programming the ESP32      |
| ![ThingSpeak](https://img.icons8.com/color/48/000000/api.png) **ThingSpeak** | Platform for data analysis and storage                 |
| ![Telegram](https://img.icons8.com/color/48/000000/telegram-app.png) **Telegram Bot** | Device management and data retrieval                  |
| OLED Display          | Displays temperature, humidity, CO₂ levels, and VOC data         |
| MH-Z19B               | CO₂ sensor for measuring carbon dioxide concentration             |
| BME680                | Environmental sensor for temperature, humidity, pressure, and VOC |

---

## 🎥 Device Demonstration

### 🚦 Air Quality Color Indication

🌈 Air quality indication via LED:

- 🟢 **Green**: Low CO₂ level (CO₂ ≤ 800 ppm)
- 🔵 **Blue**: Medium CO₂ level (800 < CO₂ ≤ 1200 ppm)
- 🔴 **Red**: High CO₂ level (CO₂ > 1200 ppm)

| Pollution Level | Indicator Color  | GIF                                                                 |
|-----------------|------------------|---------------------------------------------------------------------|
| **Low**         | 🟢 Green         | <img src="https://github.com/user-attachments/assets/5db60e05-f30b-453e-adcc-25c48f4fce59" width="33%"> |
| **Medium**      | 🔵 Blue          | <img src="https://github.com/user-attachments/assets/6d33839c-0082-4a2e-b6f4-b180ba2322fd" width="33%"> |
| **High**        | 🔴 Red           | <img src="https://github.com/user-attachments/assets/316a3ae8-afe6-4889-a398-91eec88e01a7" width="33%"> |

### 📟 Example of Data Display on the OLED Screen

<img src="https://github.com/user-attachments/assets/faeafdb1-6e40-489e-a47f-1c8100d05fae" width="20%">

---

## ⚙️ Components

- 🪣 **ESP32 DevKit**
- 🔴 **MH-Z19B** CO₂ sensor
- 🌡️ **BME680** temperature, humidity, pressure, and VOC sensor
- 📺 **OLED SSD1306 display**
- 💡 **RGB LED**
- 🔗 Resistors, wires, and other components

---

## 🚀 Installation and Setup

### 1. Hardware Preparation

Assemble the circuit according to the provided schematics. Ensure all connections are secure.

### 2. Software Preparation

- Clone the repository:

  ```bash
  git clone https://github.com/CyberScopeToday/ESP32-AirQuality-Station-Home.git
  ```

- Install **Arduino IDE** and the necessary libraries (see below).

### 3. Configuration Setup

#### Installing Libraries

Ensure the following libraries are installed in the Arduino IDE:

- `WiFi.h`  
- `Wire.h`  
- `Adafruit GFX Library`  
- `Adafruit SSD1306`  
- `Adafruit BME680 Library`  
- `MHZ19`  
- `HTTPClient`  
- `UniversalTelegramBot`  
- `ArduinoJson`  
- `Preferences`

You can install these libraries via the Arduino Library Manager (`Sketch` -> `Include Library` -> `Manage Libraries...`).

#### Configuring `config.h`

Create a file named **`config.h`** in your project directory with the following content:

```cpp
#ifndef CONFIG_H
#define CONFIG_H

// Конфигурация Wi-Fi
#define WIFI_SSID "your_wifi_ssid"          // Замените на ваш SSID Wi-Fi
#define WIFI_PASSWORD "your_wifi_password"  // Замените на ваш пароль Wi-Fi

// Конфигурация ThingSpeak
#define THINGSPEAK_API_KEY "your_api_key"   // Замените на ваш API ключ ThingSpeak

// Конфигурация Telegram-бота
#define TELEGRAM_TOKEN "your_bot_token"     // Замените на токен вашего Telegram-бота
#define TELEGRAM_CHAT_ID "your_chat_id"     // Замените на ваш Telegram chat ID

// Пороги CO₂ для индикации RGB-светодиода
#define CO2_LOW_THRESHOLD 800              // Зеленый: CO₂ ≤ 800 ppm
#define CO2_MEDIUM_THRESHOLD 1200          // Синий: 800 < CO₂ ≤ 1200 ppm

#endif
```

### 4. Uploading the Sketch

1. **Откройте Arduino IDE** и загрузите файл `main.ino`.
2. **Выберите правильную плату и COM-порт:**
   - `Инструменты` > `Плата` > выберите вашу модель ESP32 (например, **"ESP32 Dev Module"**).
   - `Инструменты` > `Порт` > выберите соответствующий COM-порт.
3. **Загрузите скетч** на устройство, нажав кнопку **"Загрузить"** (стрелка вправо).

---

## 📱 Telegram Bot Commands

| Команда                        | Описание                                                                 |
|--------------------------------|-------------------------------------------------------------------------|
| `/start`                       | Приветственное сообщение и помощь                                       |
| `/status`                      | Получение текущих данных с устройства                                   |
| `/setapikey <api_key>`         | Установка нового API ключа для ThingSpeak                               |
| `/setinterval <секунды>`       | Установка интервала отправки данных (минимум 300 секунд)                |
| `/setthreshold <низкий> <средний>` | Установка пороговых значений CO₂                                |
| `/getsettings`                 | Просмотр текущих настроек                                               |
| `/toggledebug`                 | Включение/выключение отладочной информации на OLED-дисплее              |

---

## 📸 Device Photos

<img src="https://github.com/user-attachments/assets/1c6346f7-507c-4254-9419-6f46fdc3cfb8" width="40%">

---

## 📈 Data Visualization on ThingSpeak

All data collected by the ESP32 Air Quality Monitoring Station is sent to **ThingSpeak**, a cloud-based platform for IoT data visualization and analysis.

### 🌟 Live Dashboard Example

Explore a real-time example of air quality data visualization on ThingSpeak:  
**[ThingSpeak Channel - Air Quality Monitor](https://thingspeak.mathworks.com/channels/971678)**

### 📊 Charts Available:

- 🌡️ **Temperature**: Visualize ambient temperature from **BME680** sensor.
- 💧 **Humidity**: Monitor real-time humidity levels measured by the **BME680** sensor.
- 💨 **CO₂ Concentration**: Track CO₂ levels with data from **MH-Z19B**.
- 🌫️ **VOC**: Monitor Volatile Organic Compounds with **BME680**.

---

### 🔗 How to Access

1. Open the **[ThingSpeak Channel](https://thingspeak.mathworks.com/channels/971678)**.
2. Use the **charts** to view real-time air quality data updated every 5 minutes.
3. Analyze trends, download datasets, or integrate the data into your own applications using ThingSpeak's API.

---

## 📄 License

This project is licensed under the **MIT** License. See [LICENSE](LICENSE) for details.

---

## 🎯 Useful Links

- 📕 [ESP32 Documentation](https://www.espressif.com/en/products/socs/esp32/resources)
- 🌐 [ThingSpeak API](https://thingspeak.com)
- 🤖 [Creating a Telegram Bot](https://core.telegram.org/bots)

---

# 🇷🇺 Русская версия README

You can find the Russian version of this README [here](readmeru.md).

---
