#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

#define DHTPIN 21
#define DHTTYPE DHT22

// -------- Config --------
const unsigned long PUBLISH_INTERVAL = 5000;   // ms
const char* WIFI_SSID     = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

// FIWARE-ish topic layout (Ultralight 2.0 style)
const char* SERVICE_PATH   = "TEF";
const char* DEVICE_ID      = "esp32-chronos01";
const char* MQTT_BROKER    = "YOUR_PUBLIC_IP";
const int   MQTT_PORT      = 1883;

// Topics
String topicAttrs   = String("/") + SERVICE_PATH + "/" + DEVICE_ID + "/attrs";
String topicCmd     = String("/") + SERVICE_PATH + "/" + DEVICE_ID + "/cmd";
String topicStatus  = String("/") + SERVICE_PATH + "/" + DEVICE_ID + "/status";

const char* MQTT_CLIENT_ID = "fiware_001";

WiFiClient espClient;
PubSubClient mqtt(espClient);
DHT dht(DHTPIN, DHTTYPE);

unsigned long lastPublish = 0;

// --- Utils ---
void logInfo(const String& s){ Serial.print("[INFO] "); Serial.println(s); }
void logError(const String& s){ Serial.print("[ERROR] "); Serial.println(s); }

// --- WiFi / MQTT ---
void initWiFi() {
  Serial.print("Connecting to WiFi ");
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) { delay(300); Serial.print("."); }
  Serial.print(" connected, IP="); Serial.println(WiFi.localIP());
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg; msg.reserve(length);
  for (unsigned int i=0;i<length;i++) msg += (char)payload[i];
  msg.trim();
  logInfo("CMD on " + String(topic) + " -> '" + msg + "'");
}

bool connectMQTT() {
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(mqttCallback);

  // LWT: if the client dies, broker publishes "offline" retained
  if (mqtt.connect(MQTT_CLIENT_ID, topicStatus.c_str(), 0, true, "offline")) {
    // Mark online (retained)
    mqtt.publish(topicStatus.c_str(), "online", true);
    mqtt.subscribe(topicCmd.c_str());
    logInfo("MQTT connected, subscribed to " + topicCmd);
    return true;
  } else {
    logError("MQTT connect failed, rc=" + String(mqtt.state()));
    return false;
  }
}

void ensureConnections() {
  if (WiFi.status() != WL_CONNECTED) initWiFi();
  if (!mqtt.connected()) {
    while (!connectMQTT()) { delay(2000); }
  }
}

// --- Telemetry ---
void publishTelemetryUltralight() {
  float t = dht.readTemperature(); // C
  float h = dht.readHumidity();    // %

  // Build Ultralight payload: t|<val>|h|<val>
  String payload = "t|";
  payload += (isnan(t) ? "null" : String(t, 1));
  payload += "|h|";
  payload += (isnan(h) ? "null" : String(h, 1));

  mqtt.publish(topicAttrs.c_str(), payload.c_str());
  logInfo("PUB " + topicAttrs + " -> " + payload);
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  initWiFi();
  connectMQTT();
  logInfo("Device: " + String(DEVICE_ID) + " Interval: " + String(PUBLISH_INTERVAL) + " ms");
}

void loop() {
  ensureConnections();
  mqtt.loop();

  unsigned long now = millis();
  if (now - lastPublish >= PUBLISH_INTERVAL) {
    lastPublish = now;
    publishTelemetryUltralight();
  }
}
