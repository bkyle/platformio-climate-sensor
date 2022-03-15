#include "Interval.h"
#include <Arduino.h>

Interval::Interval(long interval) {
    _interval = interval;
    _timeout = millis() + _interval;
}

bool Interval::check() {
    long ms = millis();
    bool elapsed = ms > _timeout;
    if (elapsed) {
        _timeout = ms + _interval;
    }
    return elapsed;
}