#ifndef CONFIG_H
#define CONFIG_H

// Wi-Fi Configuration
#define WIFI_SSID "your_wifi_ssid"          // Replace with your Wi-Fi SSID
#define WIFI_PASSWORD "your_wifi_password"  // Replace with your Wi-Fi Password

// ThingSpeak Configuration
#define THINGSPEAK_API_KEY "your_api_key"   // Replace with your ThingSpeak API Key

// Telegram Bot Configuration
#define TELEGRAM_TOKEN "your_bot_token"     // Replace with your Telegram bot token
#define TELEGRAM_CHAT_ID "your_chat_id"     // Replace with your Telegram chat ID

// CO2 Thresholds for RGB LED Indication
#define CO2_LOW_THRESHOLD 800              // Green: CO₂ ≤ 800 ppm
#define CO2_MEDIUM_THRESHOLD 1200          // Blue: 800 < CO₂ ≤ 1200 ppm

#endif
