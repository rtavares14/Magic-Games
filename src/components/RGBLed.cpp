#include "RGBLed.h"
#include "pins.h"  // This file contains PIN_RED, PIN_GREEN, PIN_BLUE

RGBLed::RGBLed() { }

void RGBLed::begin() {
    pinMode(PIN_RED, OUTPUT);
    pinMode(PIN_GREEN, OUTPUT);
    pinMode(PIN_BLUE, OUTPUT);
}

void RGBLed::setColor(uint8_t r, uint8_t g, uint8_t b) {
    analogWrite(PIN_RED, r);
    analogWrite(PIN_GREEN, g);
    analogWrite(PIN_BLUE, b);
}

void RGBLed::blink(uint8_t r, uint8_t g, uint8_t b, int delayTime) {
    setColor(r, g, b);
    delay(delayTime);
    setColor(0, 0, 0);
    delay(delayTime);
}
