#include "KeyLed.h"
#include <stdio.h>

KeyLed::KeyLed(uint8_t stbPin, uint8_t clkPin, uint8_t dioPin)
    : tm(stbPin, clkPin, dioPin) {}

void KeyLed::begin() {
    tm.displayBegin();
}

uint8_t KeyLed::readButtons() {
    return tm.readButtons();
}

void KeyLed::setLED(uint8_t index, bool state) {
    tm.setLED(index, state ? 1 : 0);
}

void KeyLed::displayTime(uint32_t elapsed, uint32_t totalDuration, int attemptCount) {
    unsigned long remaining = (totalDuration > elapsed) ? (totalDuration - elapsed) : 0;
    unsigned int secondsRemaining = remaining / 1000;
    int minutes = secondsRemaining / 60;
    int seconds = secondsRemaining % 60;
    
    char clockStr[6];  // Increase size to 6 to hold "MM.SS" + null terminator.
    sprintf(clockStr, "%02d.%02d", minutes, seconds);  // Insert a dot between minutes and seconds.
    
    char attemptChar = (attemptCount < 10) ? ('0' + attemptCount) : '9';
    char disp[9];
    sprintf(disp, "%s   %c", clockStr, attemptChar);
    tm.displayText(disp);
}
