#ifndef RGBLED_H
#define RGBLED_H

#include <Arduino.h>

class RGBLed {
public:
    RGBLed();
    void begin();
    void setColor(uint8_t r, uint8_t g, uint8_t b);
    // Cycles through a rainbow of colors for the specified duration (in milliseconds).
    void loadingEffect(unsigned long duration);
    // Optional blink method.
    void blink(uint8_t r, uint8_t g, uint8_t b, int delayTime);
};

#endif
