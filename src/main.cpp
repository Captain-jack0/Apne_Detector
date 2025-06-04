#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "Oximeter.h"
#include "time.h"

/// --- MQTT Callback ---------------------------------------------------------
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Konu: "); Serial.println(topic);
  Serial.print("Mesaj: ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

/// --- Wi-Fi & MQTT Settings -------------------------------------------------
// Wi-Fi
const char* ssid     = "Captain Mery";
const char* password = "Kamuran.1907";

// HiveMQ Cloud bilgileri (örnek)
const char* mqttHost = "a2e67bcac3454807b0255244f98b97dc.s1.eu.hivemq.cloud";
const int   mqttPort = 8883;
const char* mqttUser = "ESP32_Data";
const char* mqttPass = "ESP32_Data";
const char* topic    = "sensors/spo2/data";

/// --- HiveMQ Cloud Root CA (PEM format) -------------------------------------
// HiveMQ Cloud’un CA sertifikasını dashboard’dan alıp buraya yapıştırın:
static const char* root_ca PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";

/// --- NTP -------------------------------------------------------------------
const char* ntpServer      = "pool.ntp.org";
const long  gmtOffset_sec  = 3 * 3600;
const int   daylightOffset = 0;

// Secure client & MQTT client
WiFiClientSecure secureClient;
PubSubClient     client(secureClient);
Oximeter         ox(21, 22);

unsigned long lastMillis = 0;
const unsigned long interval = 1000;  // 1 saniye

void setup() {
  Serial.begin(115200);

  // --- Wi-Fi Bağlantısı
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi OK");

  // --- Zaman senkronizasyonu
  configTime(gmtOffset_sec, daylightOffset, ntpServer);

  // --- Sertifika yükle
  secureClient.setCACert(root_ca);

  // --- MQTT sunucusu & callback
  client.setServer(mqttHost, mqttPort);
  client.setCallback(mqttCallback);

  // --- Oksimetre başlatma
  if (!ox.begin()) {
    Serial.println("Sensor init failed!");
    while (1);
  }
  ox.startTasks();
}

void loop() {
  // 1) Bağlan ve abone olma
  if (!client.connected()) {
    Serial.print("MQTT bağlanıyor...");
    if (client.connect("ESP32_Client", mqttUser, mqttPass)) {
      Serial.println("Bağlandı");
      client.subscribe(topic);
    } else {
      Serial.print("Başarısız, rc=");
      Serial.println(client.state());
      delay(2000);
      return;  // loop’u sonlandırıp yeniden deneyelim
    }
  }
  client.loop();  // callback’lerin tetiklenmesi için

  // 2) Sensör verisi oku ve publish et
  unsigned long now = millis();
  if (now - lastMillis >= interval) {
    lastMillis = now;

    int spo2 = ox.getSpO2();
    int hr   = ox.getHeartRate();

    // Zaman damgası
    struct tm t;
    getLocalTime(&t);
    char ts[20];
    snprintf(ts, sizeof(ts),
             "%04d-%02d-%02d %02d:%02d:%02d",
             t.tm_year+1900, t.tm_mon+1, t.tm_mday,
             t.tm_hour, t.tm_min, t.tm_sec);

    // JSON mesaj
    char msg[128];
    snprintf(msg, sizeof(msg),
             "{\"timestamp\":\"%s\",\"SPO2\":%d,\"BPM\":%d}",
             ts, spo2, hr);

    Serial.println(msg);
    client.publish(topic, msg);
  }
}
