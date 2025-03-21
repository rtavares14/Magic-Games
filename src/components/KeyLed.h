#ifndef KEYLED_H
#define KEYLED_H

#include <Arduino.h>
#include <TM1638plus.h>

class KeyLed {
public:
    KeyLed(uint8_t stbPin, uint8_t clkPin, uint8_t dioPin);
    void begin();
    uint8_t readButtons();
    void setLED(uint8_t index, bool state);
    // Display a formatted timer string: time remaining and attempt count.
    void displayTime(uint32_t elapsed, uint32_t totalDuration, int attemptCount);
    
private:
    TM1638plus tm;
};

#endif
