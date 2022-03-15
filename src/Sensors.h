#pragma once
#include <Adafruit_AHTX0.h>

class Sensors {
  private:
    Adafruit_AHTX0* _AHT;
    sensors_event_t _temperature;
    sensors_event_t _humidity;
  public:
    float getTemperature();
    sensors_event_t& getTemperatureEvent();

    float getHumidity();
    sensors_event_t& getHumidityEvent();

    String toString();

    bool begin(Adafruit_AHTX0* AHT);
    bool loop();
};

// extern SensorsClass Sensors;