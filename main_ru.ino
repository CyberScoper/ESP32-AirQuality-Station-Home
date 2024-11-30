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

// OLED дисплей настройки
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1  // Default reset pin
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// RGB LED пины
#define RED_PIN    16
#define GREEN_PIN  17
#define BLUE_PIN    5

// Инициализация BME680
Adafruit_BME680 bme;

// Инициализация MH-Z19B (UART2 на ESP32)
HardwareSerial mySerial(2);
MHZ19 myMHZ19;

// Telegram Bot Setup
WiFiClientSecure secured_client;
UniversalTelegramBot bot(TELEGRAM_TOKEN, secured_client);

// Preferences для хранения настроек
Preferences preferences;

// Переменные для Telegram Bot
unsigned long bot_lasttime = 0;
const unsigned long BOT_MTBS = 1000;  // 1 секунда интервала

// Переменные для хранения данных
float temperature = 0.0;
float humidity = 0.0;
float pressure = 0.0;
float pressure_mmHg = 0.0; // Давление в мм рт. ст.
float gas = 0.0; // VOC
int co2 = 0;

// Временные переменные
unsigned long previousMillis = 0;
unsigned long interval = 300000;  // 5 минут
const unsigned long MIN_INTERVAL = 300000; // 5 минут

// Переменные для мигания LED
unsigned long previousLedMillis = 0;
int ledInterval = 0;

// Пороги CO2
int co2_low = CO2_LOW_THRESHOLD;
int co2_medium = CO2_MEDIUM_THRESHOLD;

// Флаг для отображения отладочной информации
bool showDebugInfo = true;

// Переменные для ThingSpeak
String thingSpeakApiKey = THINGSPEAK_API_KEY;

// Функции
void readSensors();
void blinkRGBLed(int co2_value);
void displayData();
void sendDataToThingSpeak();
void handleNewMessages(int numNewMessages);
void loadSettings();

void setup() {
  Serial.begin(115200);
  
  // Инициализация RGB LED пинов
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  
  // Инициализация OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Адрес 0x3C для 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Не продолжать, зациклиться навсегда
  }
  display.setRotation(2); // Поворот дисплея на 180 градусов, если нужно
  display.clearDisplay();
  display.display();
  
  // Инициализация BME680
  if (!bme.begin()) {
    Serial.println("Не удалось найти действительный сенсор BME680, проверьте подключение!");
    while (1);
  }
  
  // Настройка BME680
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320°C на 150 мс
  
// Initialize MH-Z19B (UART2 on ESP32)
mySerial.begin(9600, SERIAL_8N1, 4, 15); // RX=4, TX=15
myMHZ19.begin(mySerial);
myMHZ19.autoCalibration();

  // Подключение к Wi-Fi
  Serial.print("Подключение к Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Подключено");
  
  // Получение и отображение IP адреса
  String deviceIP = WiFi.localIP().toString();
  Serial.print("IP адрес ESP32: ");
  Serial.println(deviceIP);
  
  // Настройка Telegram клиента
  secured_client.setInsecure(); // Отключение проверки SSL сертификатов (не рекомендуется для продакшн)
  
  // Инициализация Preferences
  preferences.begin("settings", false); // Namespace "settings", режим чтения-записи
  loadSettings();
  
  Serial.println("Настройка завершена");
}

void loop() {
  readSensors();
  blinkRGBLed(co2);
  displayData();
  
  // Отправка данных на ThingSpeak каждые 'interval' миллисекунд
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    Serial.println("Отправка данных на ThingSpeak...");
    sendDataToThingSpeak();
    previousMillis = currentMillis;
  }
  
  // Проверка новых сообщений в Telegram
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
  // Чтение данных BME680
  if (bme.performReading()) {
    temperature = bme.temperature;
    humidity = bme.humidity;
    pressure = bme.pressure / 100.0; // гПа (гектопаскали)
    pressure_mmHg = pressure * 0.75006; // Конвертируем в мм рт. ст.
    gas = bme.gas_resistance / 1000.0; // KOhms
    Serial.println("Данные BME680 успешно прочитаны.");
  } else {
    Serial.println("Не удалось выполнить чтение BME680");
  }
  
  // Чтение данных MH-Z19B CO2
  co2 = myMHZ19.getCO2();
  Serial.print("CO2: ");
  Serial.print(co2);
  Serial.println(" ppm");
}


void blinkRGBLed(int co2_value) {
  unsigned long currentMillis = millis();
  
  if (currentMillis - previousLedMillis >= ledInterval) {
    previousLedMillis = currentMillis;
    
    // Генерация случайного интервала мигания между 30 и 50 мс
    ledInterval = random(30, 50);
    
    // Переключение состояния LED
    static bool ledState = false;
    ledState = !ledState;
    
    if (co2_value <= co2_low) {
      // Зеленый
      digitalWrite(RED_PIN, LOW);
      digitalWrite(GREEN_PIN, ledState ? HIGH : LOW);
      digitalWrite(BLUE_PIN, LOW);
    }
    else if (co2_value > co2_low && co2_value <= co2_medium) {
      // Синий
      digitalWrite(RED_PIN, LOW);
      digitalWrite(GREEN_PIN, LOW);
      digitalWrite(BLUE_PIN, ledState ? HIGH : LOW);
    }
    else {
      // Красный
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
  
  // Отображение температуры
  display.setCursor(0, 0);
  display.print("Temp: ");
  display.print(temperature);
  display.println(" C");
  
  // Отображение влажности
  display.print("Hum: ");
  display.print(humidity);
  display.println(" %");
  
  // Отображение давления в мм рт. ст.
  display.print("Press: ");
  display.print(pressure_mmHg);
  display.println(" mmHg");
  
  // Отображение VOC
  display.print("VOC: ");
  display.print(gas);
  display.println(" KOhms");
  
  // Отображение CO2
  display.print("CO2: ");
  display.print(co2);
  display.println(" ppm");
  
  // Отображение IP адреса, если включен режим отладки
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
    Serial.println("Ошибка: Wi-Fi не подключено");
  }
}

void handleNewMessages(int numNewMessages) {
  Serial.println("Получены новые сообщения");
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;
    String from_name = bot.messages[i].from_name;
    
    if (chat_id != String(TELEGRAM_CHAT_ID)) {
      bot.sendMessage(chat_id, "У вас нет доступа к этому боту.", "");
      continue;
    }
    
    if (text == "/start") {
      String welcome = "Здравствуйте, " + from_name + ".\n";
      welcome += "Используйте команду /status для получения текущих данных.";
      bot.sendMessage(chat_id, welcome, "");
    }
    else if (text == "/status") {
      String message = "Текущие данные качества воздуха:\n";
      message += "Температура: " + String(temperature) + " C\n";
      message += "Влажность: " + String(humidity) + " %\n";
      message += "Давление: " + String(pressure) + " hPa\n";
      message += "VOC: " + String(gas) + " KOhms\n";
      message += "CO2: " + String(co2) + " ppm\n";
      bot.sendMessage(chat_id, message, "");
    }
    else if (text.startsWith("/setapikey ")) {
      String newApiKey = text.substring(11);
      thingSpeakApiKey = newApiKey;
      preferences.putString("apiKey", thingSpeakApiKey);
      bot.sendMessage(chat_id, "ThingSpeak API Key обновлен.", "");
      Serial.println("ThingSpeak API Key обновлен через бот");
    }
    else if (text.startsWith("/setinterval ")) {
      String intervalStr = text.substring(13);
      unsigned long newInterval = intervalStr.toInt() * 1000; // Конвертация секунд в миллисекунды
      if (newInterval >= MIN_INTERVAL) {
        interval = newInterval;
        preferences.putULong("interval", interval);
        bot.sendMessage(chat_id, "Интервал обновления установлен на " + String(interval / 1000) + " секунд.", "");
      }
      else {
        bot.sendMessage(chat_id, "Интервал должен быть не менее " + String(MIN_INTERVAL / 1000) + " секунд.", "");
      }
    }
    else if (text.startsWith("/setthreshold ")) {
      // Ожидаемый формат: /setthreshold <низкий> <средний>
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
          bot.sendMessage(chat_id, "Пороговые значения CO₂ установлены: низкий = " + String(co2_low) + ", средний = " + String(co2_medium), "");
          Serial.println("Пороговые значения CO₂ обновлены через бот");
        }
        else {
          bot.sendMessage(chat_id, "Неверные пороговые значения. Убедитесь, что средний порог больше низкого и оба положительные.", "");
        }
      }
      else {
        bot.sendMessage(chat_id, "Неверный формат команды. Используйте: /setthreshold <низкий> <средний>", "");
      }
    }
    else if (text == "/getsettings") {
      String settings = "Текущие настройки:\n";
      settings += "ThingSpeak API Key: " + String(thingSpeakApiKey) + "\n";
      settings += "Интервал обновления: " + String(interval / 1000) + " секунд\n";
      settings += "Порог CO₂: низкий = " + String(co2_low) + ", средний = " + String(co2_medium) + "\n";
      settings += "Отладочная информация на OLED: " + String(showDebugInfo ? "Включена" : "Выключена") + "\n";
      bot.sendMessage(chat_id, settings, "");
    }
    else if (text == "/toggledebug") {
      showDebugInfo = !showDebugInfo;
      preferences.putBool("showDebugInfo", showDebugInfo);
      bot.sendMessage(chat_id, "Отладочная информация на OLED " + String(showDebugInfo ? "включена." : "выключена."), "");
      Serial.println("Отладочная информация на OLED " + String(showDebugInfo ? "включена через бот" : "выключена через бот"));
    }
    else {
      bot.sendMessage(chat_id, "Неизвестная команда. Используйте /status, /setapikey, /setinterval, /setthreshold, /getsettings или /toggledebug.", "");
    }
  }
}

void loadSettings() {
  // Загрузка ThingSpeak API Key
  String storedApiKey = preferences.getString("apiKey", thingSpeakApiKey);
  thingSpeakApiKey = storedApiKey;
  
  // Загрузка интервала обновления
  interval = preferences.getULong("interval", interval);
  
  // Загрузка порогов CO2, если они были ранее установлены
  co2_low = preferences.getInt("co2_low", CO2_LOW_THRESHOLD);
  co2_medium = preferences.getInt("co2_medium", CO2_MEDIUM_THRESHOLD);
  
  // Загрузка флага отладки
  showDebugInfo = preferences.getBool("showDebugInfo", showDebugInfo);
  
  // Убедитесь, что интервал не меньше минимального
  if (interval < MIN_INTERVAL) {
    interval = MIN_INTERVAL;
    preferences.putULong("interval", interval);
  }
  
  Serial.println("Настройки загружены:");
  Serial.println("API Key: " + thingSpeakApiKey);
  Serial.println("Интервал: " + String(interval / 1000) + " секунд");
  Serial.println("Порог CO₂: низкий = " + String(co2_low) + ", средний = " + String(co2_medium));
}
