#ifndef RGBLED_H
#define RGBLED_H

#include <Arduino.h>

class RGBLed {
public:
    RGBLed();
    void begin();
    void setColor(uint8_t r, uint8_t g, uint8_t b);
    // Optionally, add a blink method if needed.
    void blink(uint8_t r, uint8_t g, uint8_t b, int delayTime);
};

#endif
