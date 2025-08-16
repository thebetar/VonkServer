/*
  DHT Sensor with SSD1306 OLED Display

  Reads temperature and humidity from a DHT sensor and displays it on an Adafruit SSD1306 OLED screen.
  Also includes the existing LED control based on humidity.
*/
#include <stdbool.h>
#include <string.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

// --- Setup WiFi ---
#include "WiFi.h"
#include <HTTPClient.h>

// --- LED Define ---
#define RED_LED_PIN 15
#define GREEN_LED_PIN 19
#define YELLOW_LED_PIN 18
#define BLUE_LED_PIN 21

// --- Lightsensor Define ---
#define LIGHT_PIN 34 // Has to be ADC ready PIN

// --- Air quality Define
#define AIR_PIN 35
#define CO_PIN 32

// --- DHT Sensor Defines ---
#define DHTPIN 22     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22 // DHT 22 (AM2302)

DHT dht(DHTPIN, DHTTYPE);

// --- WiFi credentials
const char SSID[32] = "raspberrypi-network";
const char PASSWORD[32] = "ZuziaIsCute";

const char *server_url = "http://192.168.88.252:8080";

const uint8_t sleep_time_minutes = 50;   // Sleep time in minutes
const uint32_t uS_TO_S_FACTOR = 1000000; // Conversion factor for microseconds

const uint8_t detect_temperature_value = 27;
const uint8_t detect_humidity_value = 40;
const uint8_t detect_air_quality_value = 1000;
const uint8_t detect_co_value = 1000;

bool sensor_missread = false;

void connect_to_wifi()
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);

  Serial.println();
  Serial.println("Scanning for WiFi networks...");
  int n = WiFi.scanNetworks();

  bool found = false;
  for (int i = 0; i < n; ++i)
  {
    String s = WiFi.SSID(i);
    int rssi = WiFi.RSSI(i);
    int ch = WiFi.channel(i);
    int enc = WiFi.encryptionType(i);

    const char *encName = "UNKNOWN";
    switch (enc)
    {
    case 0:
      encName = "OPEN";
      break;
    case 1:
      encName = "WEP";
      break;
    case 2:
      encName = "WPA_PSK";
      break;
    case 3:
      encName = "WPA2_PSK";
      break;
    case 4:
      encName = "WPA/WPA2_PSK";
      break;
    case 5:
      encName = "WPA2_ENTERPRISE";
      break;
    case 6:
      encName = "WPA3_PSK";
      break;
    }

    Serial.printf("  %d: \"%s\"  RSSI:%d  CH:%d  ENC:%d (%s)\n", i, s.c_str(), rssi, ch, enc, encName);

    if (s == String(SSID))
      found = true;
  }

  if (!found)
  {
    Serial.println("Target SSID not seen in scan. It may be hidden, on 5GHz, or blocked by AP settings.");
  }

  Serial.print("Connecting to SSID: ");
  Serial.println(SSID);
  WiFi.begin(SSID, PASSWORD);

  unsigned long start = millis();
  const unsigned long timeout = 20000; // 20s
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeout)
  {
    Serial.print('.');
    delay(500);
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("Successfully connected!");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.printf("Failed to connect, status=%d\n", WiFi.status());
    Serial.println("Check AP security (must be WPA2-PSK) and 2.4GHz, MAC filter, client limit, power.");
  }
}

void setup()
{
  Serial.begin(9600);

  connect_to_wifi();

  dht.begin(); // Initialize DHT sensor

  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);
}

void send_data(char *path, float value)
{
  if (isnan(value) || isinf(value))
  {
    Serial.printf("NaN value read from sensor %s. \n", path);
    sensor_missread = true;
    return;
  }

  static char full_url[128];
  sprintf(full_url, "%s%s", server_url, path);

  // Parse data to be send to string
  char postData[16];
  sprintf(postData, "%.2f", value);

  int attempt = 0;
  bool success = false;

  while (attempt < 3 && !success)
  {
    // Parse URL and start request
    HTTPClient http;
    http.begin(full_url);

    // Add basic header
    http.addHeader("Content-Type", "text/plain");

    Serial.printf("[HTTP] POST sending data %s\n", postData);

    // Send request
    int httpResponseCode = http.POST(postData);

    // Check result
    if (httpResponseCode >= 200 && httpResponseCode < 300)
    {
      Serial.printf("[HTTP] POST... code: %d\n", httpResponseCode);
      String response = http.getString();
      Serial.println(response);
      success = true;
    }
    else
    {
      Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpResponseCode).c_str());
      delay(2000);
    }
    http.end();
  }
}

void handle_sensors()
{
  // Reading temperature or humidity takes about 250 milliseconds!
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  int light = analogRead(LIGHT_PIN);
  int air_quality = analogRead(AIR_PIN);
  int co = analogRead(CO_PIN);

  // Check if any reads failed and exit early (to try again).
  if (isnan(humidity) || isnan(temperature) || isnan(light) || isnan(air_quality) || isnan(co))
  {
    Serial.println("Failed to read from DHT sensor!");
    sensor_missread = true;
    return;
  }

  // --- Serial Monitor Output ---
  Serial.print("Temp: ");
  Serial.print(temperature);
  Serial.println("C");

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");

  Serial.print("Light value: ");
  Serial.println(light);

  Serial.print("Air quality: ");
  Serial.println(air_quality);

  Serial.print("CO value: ");
  Serial.println(co);

  // --- Send Temperature to Server ---
  if (WiFi.status() == WL_CONNECTED)
  {
    // Send data
    send_data("/humidity", humidity);
    send_data("/temperature", temperature);
    send_data("/light", (float)light);
    send_data("/air_quality", (float)air_quality);
    send_data("/co", (float)co);
  }
  else
  {
    Serial.println("WiFi not connected, skipping data upload.");
  }

  int led_on = 0;

  // Light red LED if the temperature is too high
  if (temperature > detect_temperature_value)
  {
    digitalWrite(RED_LED_PIN, HIGH);
    Serial.println("Temperature LED turned on");
    led_on = 1;
  }
  else
  {
    digitalWrite(RED_LED_PIN, LOW);
  }

  // Light green LED if the humidity is too low
  if (humidity < detect_humidity_value)
  {
    digitalWrite(GREEN_LED_PIN, HIGH);
    Serial.println("Humidity LED turned on");
    led_on = 1;
  }
  else
  {
    digitalWrite(GREEN_LED_PIN, LOW);
  }

  // Light yellow LED if the air quality is too low
  if (air_quality > detect_air_quality_value)
  {
    digitalWrite(YELLOW_LED_PIN, HIGH);
    Serial.println("Air quality LED turned on");
    led_on = 1;
  }
  else
  {
    digitalWrite(YELLOW_LED_PIN, LOW);
  }

  if (co > detect_co_value)
  {
    digitalWrite(BLUE_LED_PIN, HIGH);
    Serial.println("CO LED turned on");
    led_on = 1;
  }
  else
  {
    digitalWrite(BLUE_LED_PIN, LOW);
  }

  // If the LEDs are on, add a manual delay of 2 minutes to allow the user to see the LEDs
  if (led_on)
  {
    Serial.println("LEDs are on, waiting for 2 minutes to allow user to see them.");
    delay(120000);
  }
}

const uint32_t sensor_warmup_time = 10 * 60 * 1000; // 10 minutes in milliseconds

void loop()
{
  // Delay to allow the MQ sensors to warm up after initial start or after waking up from deep sleep
  delay(sensor_warmup_time);

  sensor_missread = false;

  handle_sensors();

  if (sensor_missread)
  {
    Serial.println("Sensor missread, skipping sleep.");
    Serial.flush();

    delay(10000);
    return;
  }

  Serial.println("Going to sleep for some time...");
  Serial.flush();

  esp_sleep_enable_timer_wakeup(sleep_time_minutes * 60 * uS_TO_S_FACTOR);
  esp_deep_sleep_start();

  // The ESP32 will reset and start from the beginning of setup after waking up
}