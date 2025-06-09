/*
  DHT Sensor with SSD1306 OLED Display

  Reads temperature and humidity from a DHT sensor and displays it on an Adafruit SSD1306 OLED screen.
  Also includes the existing LED control based on humidity.
*/

#include <string.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

// --- DHT Sensor Defines ---
#define DHTPIN 22     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22 // DHT 22 (AM2302)

DHT dht(DHTPIN, DHTTYPE);

// --- Setup WiFi ---
#include "WiFi.h"
#include <HTTPClient.h>

// --- LED Define ---
#define LED_PIN 23 // Onboard LED pin for ESP32

// --- Lightsensor Define ---
#define LIGHT_PIN 34 // Has to be ADC ready PIN

#define SSID "Housemates.pl"
#define PASSWORD "8CffkmAu7knz"

const char *server_url = "http://192.168.0.234:8080";

void connect_to_wifi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);
  Serial.print("Connecting to WiFi ..");

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print('.');
    delay(1000);
  }

  Serial.println(WiFi.localIP());
}

void setup()
{
  Serial.begin(9600);

  connect_to_wifi();

  dht.begin(); // Initialize DHT sensor

  pinMode(LED_PIN, OUTPUT); // Initialize LED pin as an output
}

void send_data(char *path, float value)
{
  static char full_url[128];

  // Parse URL and start request
  HTTPClient http;
  sprintf(full_url, "%s%s", server_url, path);
  http.begin(full_url);

  // Add basic header
  http.addHeader("Content-Type", "text/plain");

  // Parse data to be send to string
  char postData[16];
  sprintf(postData, "%.2f", value);

  Serial.printf("[HTTP] POST sending data %s\n", postData);

  // Send request
  int httpResponseCode = http.POST(postData);

  // Check result
  if (httpResponseCode > 0)
  {
    Serial.printf("[HTTP] POST... code: %d\n", httpResponseCode);
    String response = http.getString();
    Serial.println(response);
  }
  else
  {
    Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpResponseCode).c_str());
  }
  http.end();
}

void loop()
{
  // Reading temperature or humidity takes about 250 milliseconds!
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  int l = analogRead(LIGHT_PIN);

  // --- Serial Monitor Output ---
  Serial.print("Temp: ");
  Serial.print(t);
  Serial.println("C");

  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.println(" %");

  Serial.print("Light value: ");
  Serial.println(l);

  // --- LED Control ---
  if (h < 35)
  {
    digitalWrite(LED_PIN, HIGH); // turn the LED on
  }
  else
  {
    digitalWrite(LED_PIN, LOW); // turn the LED off
  }

  // --- Send Temperature to Server ---
  if (WiFi.status() == WL_CONNECTED)
  { 
    // Send data
    send_data("/humidity", h);
    send_data("/temperature", t);
    send_data("/light", (float)l);
  }
  else
  {
    Serial.println("WiFi not connected, skipping data upload.");
  }

  // Send current data every 30 minutes
  delay(30 * 60 * 1000);
}