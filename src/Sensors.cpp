#include "Sensors.h"
#include <ArduinoJson.h>

float Sensors::getTemperature() {
    return _temperature.temperature;
}

sensors_event_t& Sensors::getTemperatureEvent() {
    return _temperature;
}

float Sensors::getHumidity() {
    return _humidity.relative_humidity;
}

sensors_event_t& Sensors::getHumidityEvent() {
    return _humidity;
}

String Sensors::toString() {
    String deviceId = String(F("device-")) + String(static_cast<long>(ESP.getEfuseMac()), HEX);

    DynamicJsonDocument doc(1024);
    doc[F("deviceId")] = deviceId;
    doc[F("payload")][F("temperature")][F("value")] = getTemperature();
    doc[F("payload")][F("temperature")][F("unit")] = String(F("C"));
    doc[F("payload")][F("humidity")][F("value")] = getHumidity();
    doc[F("payload")][F("humidity")][F("unit")] = String(F("%"));

    String payload;
    serializeJson(doc, payload);
    return payload;
}

bool Sensors::begin(Adafruit_AHTX0* AHT) {
    _AHT = AHT;
    return true;
}

bool Sensors::loop() {
    return _AHT->getEvent(&_humidity, &_temperature);
}