#include "KeyLed.h"
#include <stdio.h>
#include <string.h>

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
    // Calculate remaining time.
    unsigned long remaining = (totalDuration > elapsed) ? (totalDuration - elapsed) : 0;
    unsigned int secondsRemaining = remaining / 1000;
    int minutes = secondsRemaining / 60;
    int seconds = secondsRemaining % 60;
    
    // Format time as "mmss" (always 4 characters, e.g., "0512" for 5:12).
    char timeStr[5];
    sprintf(timeStr, "%02d%02d", minutes, seconds);
    
    // Format the button count as a right-aligned number in a fixed 4-character field.
    // This means a single-digit count will be printed as "   1", two digits as "  10", etc.
    char countStr[5];
    sprintf(countStr, "%4d", attemptCount);
    
    // Combine the two parts into an 8-character string.
    char disp[9];
    sprintf(disp, "%s%s", timeStr, countStr);
    
    tm.displayText(disp);
}

void KeyLed::printTimeUsed(unsigned long startTime) {
    unsigned long usedTime = millis() - startTime;
    unsigned int totalSeconds = usedTime / 1000;
    unsigned int minutes = totalSeconds / 60;
    unsigned int seconds = totalSeconds % 60;
    Serial.print("Time used: ");
    Serial.print(minutes);
    Serial.print("m and ");
    Serial.print(seconds);
    Serial.println("s");
}
