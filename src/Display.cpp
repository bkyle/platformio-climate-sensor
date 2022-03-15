#include "Display.h"
#include <WiFi.h>
#include <Adafruit_SSD1306.h>

void Display::renderStatusArea() {
    _GFX->setCursor(0, 0);
    if (WiFi.isConnected()) {
        _GFX->print(WiFi.localIP());
    } else {
        _GFX->print("Not Connected");
    }
}

void Display::renderContentArea() {
    _GFX->setCursor(0, 20);
    _GFX->print("Temperature: ");
    _GFX->print(_sensors->getTemperature());
    _GFX->println(" C");
    _GFX->println();
    _GFX->print("   Humidity: ");
    _GFX->print(_sensors->getHumidity());
    _GFX->println(" %");
}

bool Display::begin(Adafruit_SSD1306* GFX, Sensors* sensors) {
    _GFX = GFX;
    _sensors = sensors;
    return true;
}

bool Display::loop() {
    _GFX->clearDisplay();
    _GFX->setTextColor(SSD1306_WHITE);
    renderStatusArea();
    renderContentArea();
    _GFX->display();
    return true;
}
