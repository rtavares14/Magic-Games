#ifndef POTENTIOMETER_H
#define POTENTIOMETER_H

#include <Arduino.h>

class Potentiometer {
public:
    Potentiometer(uint8_t analogPin);
    int readValue();
    // Reads the raw value and maps it between the given min and max.
    int readMappedValue(int minVal, int maxVal);
    
private:
    uint8_t analogPin;
};

#endif
