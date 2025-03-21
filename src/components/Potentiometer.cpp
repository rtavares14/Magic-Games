#include "Potentiometer.h"

Potentiometer::Potentiometer(uint8_t analogPin) : analogPin(analogPin) {}

int Potentiometer::readValue() {
    return analogRead(analogPin);
}

int Potentiometer::readMappedValue(int minVal, int maxVal) {
    int raw = analogRead(analogPin);
    return map(raw, 0, 1023, minVal, maxVal);
}
