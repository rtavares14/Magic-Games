#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>

class Button {
public:
    Button(uint8_t pin, unsigned long debounceDelay = 50);
    void begin();
    void update();
    bool isPressed();
    
private:
    uint8_t pin;
    unsigned long debounceDelay;
    int stableState;
    int lastRawState;
    unsigned long lastDebounceTime;
    bool fallingEdgeDetected;
};

#endif
