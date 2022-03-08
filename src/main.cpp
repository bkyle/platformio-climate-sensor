#include <Arduino.h>
#include <SPIFFS.h>
#include <ArduinoLog.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "config.h"

char mqttBrokerHost[64];
uint16_t mqttBrokerPort;
char mqttTopic[64];

Adafruit_AHTX0 aht;
Adafruit_SSD1306 ssd1306 = Adafruit_SSD1306(128, 64);
WiFiManager wifiManager;
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

WiFiManagerParameter mqttBrokerHostParameter("mqtt_broker_host", "MQTT Broker Server", MQTT_BROKER_HOST, strlen(MQTT_BROKER_HOST));
WiFiManagerParameter mqttBrokerPortParameter("mqtt_broker_port", "MQTT Broker Port", String(MQTT_BROKER_PORT).c_str(), String(MQTT_BROKER_PORT).length());
WiFiManagerParameter mqttTopicParameter("mqtt_topic", "MQTT Topic (%s will be replaced with the device uid)", MQTT_TOPIC, strlen(MQTT_TOPIC));

unsigned long reportTimeoutMillis = 0;

uint32_t getChipID() {
  uint32_t chipId = 0;
  for (int i=0; i<17; i++) {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }
  return chipId;
}

void callback(char *topic, byte *payload, uint32_t length) {
  char *msg = (char*) malloc(sizeof(char) * length + 1);
  strncpy(msg, (char*)payload, length);
  msg[length] = 0;
  Log.infoln("MESSAGE %s", msg);
  free(msg);
}

bool timeoutExceeded(unsigned long startMillis, unsigned long timeoutMillis) {
  return (millis() - startMillis > timeoutMillis);
}

bool setupWiFi() {
  bool ok;

  // Set to APSTA mode so that wm.autoConnect() can start an access point if there
  // isn't a persisted Wifi configuration.
  ok = WiFi.mode(WIFI_MODE_APSTA);
  if (!ok) {
    return false;
  }

  WiFiManager wm;
  wm.resetSettings();
  ok = wm.autoConnect();
  if (!ok) {
    return false;
  }

  ok = WiFi.disconnect();
  if (!ok) {
    return false;
  }

  ok = WiFi.mode(WIFI_MODE_STA);
  if (!ok) {
    return false;
  }

  ok = WiFi.begin();
  if (!ok) {
    return false;
  }

  return true;
}

bool loadDefaultConfig() {
  strlcpy(mqttBrokerHost, MQTT_BROKER_HOST, sizeof(mqttBrokerHost));
  mqttBrokerPort = MQTT_BROKER_PORT;
  strlcpy(mqttTopic, MQTT_TOPIC, sizeof(mqttTopic));
  return true;
}

bool loadConfig() {
  File file = SPIFFS.open("/config.json", FILE_READ);
  if (!file) {
    return false;
  }

  DynamicJsonDocument doc(1024);
  DeserializationError err = deserializeJson(doc, file);
  if (!err.code()) {
    file.close();
    Log.errorln(err.f_str());
    return false;
  }

  file.close();

  serializeJsonPretty(doc, Serial);

  strlcpy(mqttBrokerHost, doc["mqtt"]["broker"]["host"] | MQTT_BROKER_HOST, sizeof(mqttBrokerHost));
  mqttBrokerPort = doc["mqtt"]["broker"]["port"] | MQTT_BROKER_PORT;
  strlcpy(mqttTopic, doc["mqtt"]["topic"] | MQTT_TOPIC, sizeof(mqttTopic));

  return true;
}

bool saveConfig() {
  DynamicJsonDocument doc(1024);
  doc["mqtt"]["broker"]["host"] = mqttBrokerHost;
  doc["mqtt"]["broker"]["port"] = mqttBrokerPort;
  doc["mqtt"]["topic"] = mqttTopic;
  
  File file = SPIFFS.open("/config.json", FILE_WRITE);
  if (!file) {
    return false;
  }

  serializeJson(doc, file);
  file.close();
  return true;
}

String generateMqttClientId() {
  String clientId = "ESP32-";
  clientId += String((long)ESP.getEfuseMac(), HEX);
  clientId += "-";
  clientId += String(random(0xffff), HEX);
  return clientId;
}

bool reconnect() {
  int retries = 0;
  while (!mqttClient.connected()) {
    String clientId = generateMqttClientId();
    if (!mqttClient.connect(clientId.c_str())) {
      retries++;
      if (retries > MQTT_MAX_RETRIES) {
        break;
      }
      delay(5000);
    }
  }
  return mqttClient.connected();
}

bool setupMqttClient() {
  mqttClient.setServer(mqttBrokerHost, mqttBrokerPort);
  mqttClient.setCallback(callback);

  if (!mqttClient.connected()) {
    reconnect();
  }

  return true;
}

bool setupWiFiManager() {
  // wifiManager.resetSettings();
  wifiManager.addParameter(&mqttBrokerHostParameter);
  wifiManager.addParameter(&mqttBrokerPortParameter);
  wifiManager.addParameter(&mqttTopicParameter);
  wifiManager.setSaveConfigCallback(saveConfig);
  // wifiManager.setTitle("");
  wifiManager.autoConnect();
  // // wifiManager.autoConnect(ssid, password);
  // wifiManager.setConfigPortalBlocking(false);
  // wifiManager.setConfigPortalTimeout(60);
  // wifiManager.startConfigPortal();
  return true;
}

void listDirectory(const char *path) {
  File root = SPIFFS.open(path);
  if (!root.isDirectory()) {
    Serial.printf("%s is not a directory\n", path);
    return;
  }

  Serial.println("BEGIN");
  File cur = root;
  while (cur = cur.openNextFile()) {
    Serial.printf("%s: %s, %d\n", cur.isDirectory() ? "DIR" : "FILE", cur.name(), cur.size());
  }
  Serial.println("END");
}

void setup() {
  bool ok;
  // randomSeed(micros());

  // wifiManager.resetSettings();

  Serial.begin(115200);

  Log.begin(LOG_LEVEL_INFO, &Serial);

  // ok = SPIFFS.begin(true);
  // if (!ok) {
  //   Log.errorln("Failed to initialize SPIFFS");
  // }

  // ok = loadConfig();
  // if (!ok) {
  //   Log.errorln("Failed to load configuration, loading defaults");
  //   ok = loadDefaultConfig();
  //   if (!ok) {
  //     Log.fatalln("Failed to load default configuration");
  //   }
  // }

  ok = setupWiFi();
  if (!ok) {
    Log.errorln("Unable to establish WiFi connection");
  }

  // ok = setupWiFiManager();
  // if (!ok) {
  //   Log.errorln("Unable to setup WiFi Manager");
  // }

  // ok = setupMqttClient();
  // if (!ok) {
  //   Log.errorln("Unable to establish MQTT connection");
  // }

  ok = Wire.setPins(SDA, SCL);
  if (!ok) {
    Log.fatalln("I2C pin configuration failed");
  }

  ok = Wire.begin();
  if (!ok) {
    Log.fatalln("I2C initialized failed");
  }

  ok = aht.begin();
  if (!ok) {
    Log.fatalln("AHT not available");
  }

  ok = ssd1306.begin(SSD1306_SWITCHCAPVCC, 0x3C, false, true);
  if (!ok) {
    Log.fatalln("SSD1306 not available");
  }

}

void logEvent(sensors_event_t &humidity, sensors_event_t &temperature) {
  Serial.printf(
    "Temperature:%f Humidity:%f\n",
    temperature.temperature,
    humidity.relative_humidity
  );
}

void updateDisplay(sensors_event_t &humidity, sensors_event_t &temperature) {
  ssd1306.clearDisplay();
  ssd1306.setTextColor(SSD1306_WHITE);
  ssd1306.setCursor(0, 20);

  ssd1306.print("Temperature: ");
  ssd1306.print(temperature.temperature);
  ssd1306.println(" C");
  ssd1306.println();
  ssd1306.print("   Humidity: ");
  ssd1306.print(humidity.relative_humidity);
  ssd1306.println(" %");

  ssd1306.display();
}

void loop() {
  wifiManager.process();

  // if (!WiFi.isConnected()) {
  //   setupWiFiClient();
  // }

  if (!mqttClient.connected()) {
    reconnect();
  }
  mqttClient.loop();


  if (millis() > reportTimeoutMillis) {
    sensors_event_t humidity, temperature;
    aht.getEvent(&humidity, &temperature);

    logEvent(humidity, temperature);
    updateDisplay(humidity, temperature);

    // String payload = "{\"temperature\":" + 
    //   String(temperature.temperature) + 
    //   ", \"humidity\":" + 
    //   String(humidity.relative_humidity) +
    //   "}";

    // char t[100];
    // snprintf(t, sizeof(t), mqttTopic, String(getChipID(), HEX).c_str());

    // if (!mqttClient.publish(t, payload.c_str())) {
    //   Log.errorln("Failed to publish readings to %s:%d %s", mqttBrokerHost, mqttBrokerPort, t);
    // }
    reportTimeoutMillis = millis() + REPORT_INTERVAL;
  }

  // FIXME: mqttClient appears to get into a state where it can no longer reconnect.
  // Disconnecting after each iteration of the loop gets mqttClient into this state
  // within a few iterations.
  //
  // Things to try:
  //   1. mqtt.loop() before disconnect to see if there's state that doesn't get cleaned
  //      up properly?
  //   2. create a new instance of PubSubClient after disconnecting to try to clear out
  //      stale state?
  //   3. Add logging to PubSubClient to see where it's timing out.  This appears to be
  //      happening inside of connect() after writing an initial header -- waiting for
  //      bytes to be available.
  //   4. Increase the timeout from the default (15 seconds?) to something extreme?
  //
  // mqttClient.disconnect();

  // delay(2000);
}
