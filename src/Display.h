#pragma once
#include <Adafruit_SSD1306.h>
#include "Sensors.h"
#include "Connectivity.h"

class Display {
  private:
    Adafruit_SSD1306* _GFX;
    Sensors* _sensors;
    Connectivity* _connectivity;
  protected:
    void renderStatusArea();
    void renderContentArea();
  public:
    bool begin(Adafruit_SSD1306* GFX, Sensors* sensors, Connectivity* connectivity);
    bool loop();
};