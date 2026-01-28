#include <WiFi.h>
#include <HTTPClient.h>
#include <ESPmDNS.h>
#include <DHT.h>

#define RED_LED_PIN 15
#define GREEN_LED_PIN 19
#define YELLOW_LED_PIN 18
#define BLUE_LED_PIN 21
#define LIGHT_PIN 34
#define AIR_PIN 35
#define CO_PIN 32
#define DHTPIN 22
#define DHTTYPE DHT22

const char* ssid = "raspberrypi-network";
const char* password = "ZuziaIsCute";

const char* server_hostname = "raspberrypi"; 
const int server_port = 8080;

const uint8_t sleep_time_minutes = 50;
const uint32_t uS_TO_S_FACTOR = 1000000;
const uint32_t sensor_warmup_time = 10 * 60 * 1000; 

const float threshold_temp = 27.0;
const float threshold_hum = 40.0;
const int threshold_air = 1000;
const int threshold_co = 1000;

DHT dht(DHTPIN, DHTTYPE);
bool sensor_missread = false;

void setup_network() {
  Serial.println("\n--- Network Initialization ---");
  Serial.print("Connecting to WiFi SSID: ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < 20000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WiFi] Status: Connected");
    Serial.print("[WiFi] IP Address: ");
    Serial.println(WiFi.localIP());

    if (!MDNS.begin("esp32-sensor-node")) {
      Serial.println("[mDNS] Error setting up responder!");
    } else {
      Serial.println("[mDNS] Responder started successfully");
    }
  } else {
    Serial.println("\n[WiFi] Connection Failed. Check credentials or signal.");
  }
}

void send_data(const char* path, float value) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[HTTP] Upload skipped: WiFi not connected.");
    return;
  }

  Serial.printf("[mDNS] Resolving %s.local for %s...\n", server_hostname, path);
  
  IPAddress serverIP = MDNS.queryHost(server_hostname, 2000);
  
  String url;
  if (serverIP.toString() != "0.0.0.0") {
    url = "http://" + serverIP.toString() + ":" + String(server_port) + path;
  } else {
    url = "http://" + String(server_hostname) + ".local:" + String(server_port) + path;
  }

  char postData[16];
  sprintf(postData, "%.2f", value);

  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "text/plain");

  Serial.printf("[HTTP] POSTing to %s | Value: %s\n", url.c_str(), postData);
  
  int httpResponseCode = http.POST(postData);

  if (httpResponseCode > 0) {
    Serial.printf("[HTTP] POST Response Code: %d\n", httpResponseCode);
    if (httpResponseCode >= 200 && httpResponseCode < 300) {
      Serial.println("[HTTP] Success: Data accepted.");
    }
  } else {
    Serial.printf("[HTTP] Request failed, error: %s\n", http.errorToString(httpResponseCode).c_str());
  }
  
  http.end();
}

void handle_sensors() {
  Serial.println("\n--- Starting Sensor Read Cycle ---");
  
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  int l = analogRead(LIGHT_PIN);
  int a = analogRead(AIR_PIN);
  int c = analogRead(CO_PIN);

  if (isnan(h) || isnan(t)) {
    Serial.println("[Sensor] Critical Error: Failed to read from DHT sensor!");
    sensor_missread = true;
    return;
  }

  Serial.println("[Sensor] Data Captured:");
  Serial.printf("  > Temperature: %.2f C\n", t);
  Serial.printf("  > Humidity:    %.2f %%\n", h);
  Serial.printf("  > Light Level: %d\n", l);
  Serial.printf("  > Air Quality: %d\n", a);
  Serial.printf("  > CO Level:    %d\n", c);

  if (WiFi.status() == WL_CONNECTED) {
    send_data("/temperature", t);
    send_data("/humidity", h);
    send_data("/light", (float)l);
    send_data("/air_quality", (float)a);
    send_data("/co", (float)c);
  }

  bool any_led_on = false;

  if (t > threshold_temp) { 
    digitalWrite(RED_LED_PIN, HIGH); 
    Serial.println("[Alert] Temperature exceeds threshold! RED LED ON.");
    any_led_on = true; 
  } else { 
    digitalWrite(RED_LED_PIN, LOW); 
  }

  if (h < threshold_hum) { 
    digitalWrite(GREEN_LED_PIN, HIGH); 
    Serial.println("[Alert] Humidity below threshold! GREEN LED ON.");
    any_led_on = true; 
  } else { 
    digitalWrite(GREEN_LED_PIN, LOW); 
  }

  if (a > threshold_air) { 
    digitalWrite(YELLOW_LED_PIN, HIGH); 
    Serial.println("[Alert] Air quality poor! YELLOW LED ON.");
    any_led_on = true; 
  } else { 
    digitalWrite(YELLOW_LED_PIN, LOW); 
  }

  if (c > threshold_co) { 
    digitalWrite(BLUE_LED_PIN, HIGH); 
    Serial.println("[Alert] CO levels high! BLUE LED ON.");
    any_led_on = true; 
  } else { 
    digitalWrite(BLUE_LED_PIN, LOW); 
  }

  if (any_led_on) {
    Serial.println("[Wait] Thresholds reached. Holding for 2 minutes for visual check...");
    delay(120000);
  }
}

void setup() {
  Serial.begin(115200);
  
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);
  
  dht.begin();
  setup_network();
}

void loop() {
  Serial.println("\n[System] Sensor warmup initiated...");
  delay(sensor_warmup_time);

  sensor_missread = false;
  handle_sensors();

  if (sensor_missread) {
    Serial.println("[System] Sensor error detected. Retrying in 10 seconds...");
    delay(10000);
    return;
  }

  Serial.println("[System] Cycle complete. Entering Deep Sleep...");
  Serial.flush();
  
  esp_sleep_enable_timer_wakeup(sleep_time_minutes * 60 * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}