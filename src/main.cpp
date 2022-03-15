#include "config.h"
#include "Sensors.h"
#include "Interval.h"
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
Sensors sensors;
Adafruit_SSD1306 ssd1306 = Adafruit_SSD1306(128, 64);
WiFiManager wifiManager;
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

WiFiManagerParameter mqttBrokerHostParameter("mqtt_broker_host", "MQTT Broker Server", MQTT_BROKER_HOST, strlen(MQTT_BROKER_HOST));
WiFiManagerParameter mqttBrokerPortParameter("mqtt_broker_port", "MQTT Broker Port", String(MQTT_BROKER_PORT).c_str(), String(MQTT_BROKER_PORT).length());
WiFiManagerParameter mqttTopicParameter("mqtt_topic", "MQTT Topic (%s will be replaced with the device uid)", MQTT_TOPIC, strlen(MQTT_TOPIC));

Interval reportInterval(REPORT_INTERVAL);

void callback(char *topic, byte *payload, uint32_t length) {
  char *msg = (char*) malloc(sizeof(char) * length + 1);
  strncpy(msg, (char*)payload, length);
  msg[length] = 0;
  Log.infoln("MESSAGE %s", msg);
  free(msg);
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
  // wm.resetSettings();
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
  if (!err.code() == DeserializationError::Code::Ok) {
    file.close();
    Log.errorln("Failed to load configuration. %s", err.f_str());
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
  
  serializeJsonPretty(doc, Serial);
  Serial.println();

  File file = SPIFFS.open("/config.json", FILE_WRITE);
  if (!file) {
    return false;
  }

  serializeJson(doc, file);
  file.close();

  Log.infoln("Configuration saved.");

  file = SPIFFS.open(F("/config.json"), FILE_READ);
  Log.infoln("Size of config == %d", file.size());


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
  Log.infoln("Connecting to %s:%d", mqttBrokerHost, mqttBrokerPort);
  mqttClient.setServer(mqttBrokerHost, mqttBrokerPort);
  mqttClient.setCallback(callback);

  if (!mqttClient.connected()) {
    reconnect();
  }

  return true;
}

bool setupWiFiManager() {
  wifiManager.addParameter(&mqttBrokerHostParameter);
  wifiManager.addParameter(&mqttBrokerPortParameter);
  wifiManager.addParameter(&mqttTopicParameter);
  wifiManager.setSaveParamsCallback(saveConfig);
  std::vector<const char*> menu = {"wifi", "param", "info", "update"};
  wifiManager.setMenu(menu);
  wifiManager.setConfigPortalBlocking(false);
  wifiManager.startWebPortal();
  return true;
}


void setup() {
  Serial.begin(115200);

  Log.begin(LOG_LEVEL_INFO, &Serial);

  if (!SPIFFS.begin(true)) {
    Log.errorln("Failed to initialize SPIFFS");
  }

  if(!loadConfig()) {
    Log.errorln("Failed to load configuration, loading defaults");
    if (!loadDefaultConfig()) {
      Log.fatalln("Failed to load default configuration");
    }
  }

  if (!setupWiFi()) {
    Log.errorln("Unable to establish WiFi connection");
  }

  if (!setupWiFiManager()) {
    Log.errorln("Unable to setup WiFi Manager");
  }

  // if (!setupMqttClient()) {
  //   Log.errorln("Unable to establish MQTT connection");
  // }

  if (!Wire.setPins(I2C_SDA, I2C_SCL)) {
    Log.fatalln("I2C pin configuration failed");
  }

  if (!Wire.begin()) {
    Log.fatalln("I2C initialized failed");
  }

  if (!aht.begin()) {
    Log.fatalln("AHT not available");
  }

  if (!sensors.begin(aht)) {
    Log.fatalln("Sensors not available");
  }

  if (!ssd1306.begin(SSD1306_SWITCHCAPVCC, 0x3C, false, true)) {
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

void updateDisplay() {
  ssd1306.clearDisplay();
  ssd1306.setTextColor(SSD1306_WHITE);

  ssd1306.setCursor(0, 0);
  if (WiFi.isConnected()) {
    ssd1306.print(WiFi.localIP());
  } else {
    ssd1306.print("No Connection");
  }

  ssd1306.setCursor(0, 20);
  ssd1306.print("Temperature: ");
  ssd1306.print(sensors.getTemperature());
  ssd1306.println(" C");
  ssd1306.println();
  ssd1306.print("   Humidity: ");
  ssd1306.print(sensors.getHumidity());
  ssd1306.println(" %");

  ssd1306.display();
}

void loop() {
  wifiManager.process();

  // if (!WiFi.isConnected()) {
  //   setupWiFiClient();
  // }

  // if (!mqttClient.connected()) {
  //   reconnect();
  // }
  // mqttClient.loop();

  sensors.loop();

  if (reportInterval.check()) {
    Serial.println(sensors.toString());
    updateDisplay();
    // publishState(humidity, temperature);

    // String deviceId = String(F("device-")) + String(static_cast<long>(ESP.getEfuseMac()), HEX);

    // DynamicJsonDocument doc(1024);
    // doc[F("deviceId")] = deviceId;
    // doc[F("payload")][F("temperature")][F("value")] = temperature.temperature;
    // doc[F("payload")][F("temperature")][F("unit")] = String(F("C"));
    // doc[F("payload")][F("humidity")][F("value")] = humidity.relative_humidity;
    // doc[F("payload")][F("humidity")][F("unit")] = String(F("%"));

    // String payload;
    // serializeJson(doc, payload);

    // char topic[255];
    // snprintf(topic, sizeof(topic), mqttTopic, deviceId.c_str());

    // if (!mqttClient.publish(topic, payload.c_str())) {
    //   Log.errorln("Failed to publish readings to %s:%d %s", mqttBrokerHost, mqttBrokerPort, topic);
    // }
  }
}
