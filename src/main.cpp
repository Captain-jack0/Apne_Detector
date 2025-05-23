
#include <WiFi.h>
#include <PubSubClient.h>
#include "Oximeter.h"
#include "time.h"

/// Wi-Fi & MQTT

const char* ssid = "FiberHGW_ZTXE36_2.4GHz";
const char* password = "3jstqKzfsD";
const char* mqttServer = "192.168.1.38";
const int   mqttPort   = 1883;
const char* topic      = "sensors/spo2/data";

/// NTP
const char* ntpServer       = "pool.ntp.org";
const long  gmtOffset_sec   = 3 * 3600;
const int   daylightOffset  = 0;

WiFiClient   net;
PubSubClient client(net);
Oximeter     ox(21, 22);

unsigned long lastMillis = 0;
const unsigned long interval = 1000;  // 1 saniye

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nWiFi OK");

  configTime(gmtOffset_sec, daylightOffset, ntpServer);
  client.setServer(mqttServer, mqttPort);

  if (!ox.begin()) {
    Serial.println("Sensor init failed!");
    while (1);
  }
  ox.startTasks();  // sampling & processing task’leri başlat
}

String getTimestamp() {
  struct tm t;
  if (!getLocalTime(&t)) return "";
  char buf[20];
  snprintf(buf, sizeof(buf),
           "%04d-%02d-%02d %02d:%02d:%02d",
           t.tm_year+1900, t.tm_mon+1, t.tm_mday,
           t.tm_hour, t.tm_min, t.tm_sec);
  return String(buf);
}

void loop() {
  if (!client.connected()) {
    while (!client.connect("ESP32_Oxi")) {
      delay(1000);
    }
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMillis >= interval) {
    lastMillis = now;

    int spo2 = ox.getSpO2();
    int hr   = ox.getHeartRate();
    String ts = getTimestamp();

    char msg[128];
    snprintf(msg, sizeof(msg),
             "{\"timestamp\":\"%s\",\"SPO2\":%d,\"BPM\":%d}",
             ts.c_str(), spo2, hr);

    Serial.println(msg);
    client.publish(topic, msg);
  }
}
