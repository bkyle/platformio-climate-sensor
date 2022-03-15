#pragma once
#include <Adafruit_SSD1306.h>
#include "Sensors.h"

class Display {
  private:
    Adafruit_SSD1306* _GFX;
    Sensors* _sensors;
  protected:
    void renderStatusArea();
    void renderContentArea();
  public:
    bool begin(Adafruit_SSD1306* GFX, Sensors* sensors);
    bool loop();
};