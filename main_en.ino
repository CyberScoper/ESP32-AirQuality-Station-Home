#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_BME680.h>
#include <MHZ19.h>
#include <HardwareSerial.h>
#include <HTTPClient.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <Preferences.h>

#include "config.h"

// OLED display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1  // Default reset pin
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// RGB LED pins
#define RED_PIN    16
#define GREEN_PIN  17
#define BLUE_PIN    5

// Initialize BME680
Adafruit_BME680 bme;

// Initialize MH-Z19B (UART2 on ESP32)
HardwareSerial mySerial(2);
MHZ19 myMHZ19;

// Telegram Bot Setup
WiFiClientSecure secured_client;
UniversalTelegramBot bot(TELEGRAM_TOKEN, secured_client);

// Preferences for storing settings
Preferences preferences;

// Variables for Telegram Bot
unsigned long bot_lasttime = 0;
const unsigned long BOT_MTBS = 1000;  // 1 second interval

// Variables for storing data
float temperature = 0.0;
float humidity = 0.0;
float pressure = 0.0;
float pressure_mmHg = 0.0; // Pressure in mmHg
float gas = 0.0; // VOC
int co2 = 0;

// Time variables
unsigned long previousMillis = 0;
unsigned long interval = 300000;  // 5 minutes
const unsigned long MIN_INTERVAL = 300000; // 5 minutes

// Variables for LED blinking
unsigned long previousLedMillis = 0;
int ledInterval = 0;

// CO2 thresholds
int co2_low = CO2_LOW_THRESHOLD;
int co2_medium = CO2_MEDIUM_THRESHOLD;

// Flag for displaying debug information
bool showDebugInfo = true;

// Variables for ThingSpeak
String thingSpeakApiKey = THINGSPEAK_API_KEY;

// Function declarations
void readSensors();
void blinkRGBLed(int co2_value);
void displayData();
void sendDataToThingSpeak();
void handleNewMessages(int numNewMessages);
void loadSettings();

void setup() {
  Serial.begin(115200);
  
  // Initialize RGB LED pins
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  
  // Initialize OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Do not continue, loop forever
  }
  display.setRotation(2); // Rotate display by 180 degrees, if needed
  display.clearDisplay();
  display.display();
  
  // Initialize BME680
  if (!bme.begin()) {
    Serial.println("Failed to find a valid BME680 sensor, check wiring!");
    while (1);
  }
  
  // Configure BME680
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320°C for 150 ms
  
  // Initialize MH-Z19B
  mySerial.begin(9600, SERIAL_8N1, 4, 15); // RX=4, TX=15
  myMHZ19.begin(mySerial);
  myMHZ19.autoCalibration();
  
  // Connect to Wi-Fi
  Serial.print("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Connected");
  
  // Get and display IP address
  String deviceIP = WiFi.localIP().toString();
  Serial.print("ESP32 IP address: ");
  Serial.println(deviceIP);
  
  // Configure Telegram client
  secured_client.setInsecure(); // Disable SSL certificate verification (not recommended for production)
  
  // Initialize Preferences
  preferences.begin("settings", false); // Namespace "settings", read-write mode
  loadSettings();
  
  Serial.println("Setup completed");
}

void loop() {
  readSensors();
  blinkRGBLed(co2);
  displayData();
  
  // Send data to ThingSpeak every 'interval' milliseconds
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    Serial.println("Sending data to ThingSpeak...");
    sendDataToThingSpeak();
    previousMillis = currentMillis;
  }
  
  // Check for new Telegram messages
  if (WiFi.status() == WL_CONNECTED) {
    if (millis() - bot_lasttime > BOT_MTBS) {
      int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      while (numNewMessages) {
        handleNewMessages(numNewMessages);
        numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      }
      bot_lasttime = millis();
    }
  }
}

void readSensors() {
  // Read BME680 data
  if (bme.performReading()) {
    temperature = bme.temperature;
    humidity = bme.humidity;
    pressure = bme.pressure / 100.0; // hPa (hectopascals)
    pressure_mmHg = pressure * 0.75006; // Convert to mmHg
    gas = bme.gas_resistance / 1000.0; // KOhms
    Serial.println("BME680 data read successfully.");
  } else {
    Serial.println("Failed to perform BME680 reading");
  }
  
  // Read MH-Z19B CO2 data
  co2 = myMHZ19.getCO2();
  Serial.print("CO2: ");
  Serial.print(co2);
  Serial.println(" ppm");
}

void blinkRGBLed(int co2_value) {
  unsigned long currentMillis = millis();
  
  if (currentMillis - previousLedMillis >= ledInterval) {
    previousLedMillis = currentMillis;
    
    // Generate a random blink interval between 30 and 50 ms
    ledInterval = random(30, 50);
    
    // Toggle LED state
    static bool ledState = false;
    ledState = !ledState;
    
    if (co2_value <= co2_low) {
      // Green
      digitalWrite(RED_PIN, LOW);
      digitalWrite(GREEN_PIN, ledState ? HIGH : LOW);
      digitalWrite(BLUE_PIN, LOW);
    }
    else if (co2_value > co2_low && co2_value <= co2_medium) {
      // Blue
      digitalWrite(RED_PIN, LOW);
      digitalWrite(GREEN_PIN, LOW);
      digitalWrite(BLUE_PIN, ledState ? HIGH : LOW);
    }
    else {
      // Red
      digitalWrite(RED_PIN, ledState ? HIGH : LOW);
      digitalWrite(GREEN_PIN, LOW);
      digitalWrite(BLUE_PIN, LOW);
    }
  }
}

void displayData() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // Display Temperature
  display.setCursor(0, 0);
  display.print("Temp: ");
  display.print(temperature);
  display.println(" C");
  
  // Display Humidity
  display.print("Hum: ");
  display.print(humidity);
  display.println(" %");
  
  // Display Pressure in mmHg
  display.print("Press: ");
  display.print(pressure_mmHg);
  display.println(" mmHg");
  
  // Display VOC
  display.print("VOC: ");
  display.print(gas);
  display.println(" KOhms");
  
  // Display CO2
  display.print("CO2: ");
  display.print(co2);
  display.println(" ppm");
  
  // Display IP address if debug info is enabled
  if (showDebugInfo) {
    display.print("IP: ");
    display.println(WiFi.localIP().toString());
  }
  
  display.display();
}

void sendDataToThingSpeak() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String("https://api.thingspeak.com/update?api_key=") + thingSpeakApiKey +
                 "&field1=" + String(temperature) +
                 "&field2=" + String(humidity) +
                 "&field3=" + String(pressure) +
                 "&field4=" + String(gas) +
                 "&field5=" + String(co2);
    
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode > 0) {
      Serial.printf("ThingSpeak Response Code: %d\n", httpCode);
    }
    else {
      Serial.printf("ThingSpeak Error Code: %d\n", httpCode);
    }
    http.end();
  }
  else {
    Serial.println("Error: Wi-Fi not connected");
  }
}

void handleNewMessages(int numNewMessages) {
  Serial.println("Received new messages");
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;
    String from_name = bot.messages[i].from_name;
    
    if (chat_id != String(TELEGRAM_CHAT_ID)) {
      bot.sendMessage(chat_id, "You do not have access to this bot.", "");
      continue;
    }
    
    if (text == "/start") {
      String welcome = "Hello, " + from_name + ".\n";
      welcome += "Use the /status command to get current data.";
      bot.sendMessage(chat_id, welcome, "");
    }
    else if (text == "/status") {
      String message = "Current air quality data:\n";
      message += "Temperature: " + String(temperature) + " C\n";
      message += "Humidity: " + String(humidity) + " %\n";
      message += "Pressure: " + String(pressure_mmHg) + " mmHg\n";
      message += "VOC: " + String(gas) + " KOhms\n";
      message += "CO2: " + String(co2) + " ppm\n";
      bot.sendMessage(chat_id, message, "");
    }
    else if (text.startsWith("/setapikey ")) {
      String newApiKey = text.substring(11);
      thingSpeakApiKey = newApiKey;
      preferences.putString("apiKey", thingSpeakApiKey);
      bot.sendMessage(chat_id, "ThingSpeak API Key updated.", "");
      Serial.println("ThingSpeak API Key updated via bot");
    }
    else if (text.startsWith("/setinterval ")) {
      String intervalStr = text.substring(13);
      unsigned long newInterval = intervalStr.toInt() * 1000; // Convert seconds to milliseconds
      if (newInterval >= MIN_INTERVAL) {
        interval = newInterval;
        preferences.putULong("interval", interval);
        bot.sendMessage(chat_id, "Update interval set to " + String(interval / 1000) + " seconds.", "");
      }
      else {
        bot.sendMessage(chat_id, "Interval must be at least " + String(MIN_INTERVAL / 1000) + " seconds.", "");
      }
    }
    else if (text.startsWith("/setthreshold ")) {
      // Expected format: /setthreshold <low> <medium>
      int firstSpace = text.indexOf(' ');
      int secondSpace = text.indexOf(' ', firstSpace + 1);
      if (secondSpace != -1) {
        String lowStr = text.substring(firstSpace + 1, secondSpace);
        String mediumStr = text.substring(secondSpace + 1);
        int newLow = lowStr.toInt();
        int newMedium = mediumStr.toInt();
        if (newLow > 0 && newMedium > newLow) {
          co2_low = newLow;
          co2_medium = newMedium;
          preferences.putInt("co2_low", co2_low);
          preferences.putInt("co2_medium", co2_medium);
          bot.sendMessage(chat_id, "CO₂ thresholds set: low = " + String(co2_low) + ", medium = " + String(co2_medium), "");
          Serial.println("CO₂ thresholds updated via bot");
        }
        else {
          bot.sendMessage(chat_id, "Invalid threshold values. Ensure that the medium threshold is greater than the low and both are positive.", "");
        }
      }
      else {
        bot.sendMessage(chat_id, "Invalid command format. Use: /setthreshold <low> <medium>", "");
      }
    }
    else if (text == "/getsettings") {
      String settings = "Current settings:\n";
      settings += "ThingSpeak API Key: " + String(thingSpeakApiKey) + "\n";
      settings += "Update interval: " + String(interval / 1000) + " seconds\n";
      settings += "CO₂ Thresholds: low = " + String(co2_low) + ", medium = " + String(co2_medium) + "\n";
      settings += "Debug info on OLED: " + String(showDebugInfo ? "Enabled" : "Disabled") + "\n";
      bot.sendMessage(chat_id, settings, "");
    }
    else if (text == "/toggledebug") {
      showDebugInfo = !showDebugInfo;
      preferences.putBool("showDebugInfo", showDebugInfo);
      bot.sendMessage(chat_id, "Debug information on OLED " + String(showDebugInfo ? "enabled." : "disabled."), "");
      Serial.println("Debug information on OLED " + String(showDebugInfo ? "enabled via bot" : "disabled via bot"));
    }
    else {
      bot.sendMessage(chat_id, "Unknown command. Use /status, /setapikey, /setinterval, /setthreshold, /getsettings, or /toggledebug.", "");
    }
  }
}

void loadSettings() {
  // Load ThingSpeak API Key
  String storedApiKey = preferences.getString("apiKey", thingSpeakApiKey);
  thingSpeakApiKey = storedApiKey;
  
  // Load update interval
  interval = preferences.getULong("interval", interval);
  
  // Load CO2 thresholds if previously set
  co2_low = preferences.getInt("co2_low", CO2_LOW_THRESHOLD);
  co2_medium = preferences.getInt("co2_medium", CO2_MEDIUM_THRESHOLD);
  
  // Load debug flag
  showDebugInfo = preferences.getBool("showDebugInfo", showDebugInfo);
  
  // Ensure interval is not less than minimum
  if (interval < MIN_INTERVAL) {
    interval = MIN_INTERVAL;
    preferences.putULong("interval", interval);
  }
  
  Serial.println("Settings loaded:");
  Serial.println("API Key: " + thingSpeakApiKey);
  Serial.println("Interval: " + String(interval / 1000) + " seconds");
  Serial.println("CO₂ Thresholds: low = " + String(co2_low) + ", medium = " + String(co2_medium));
}
