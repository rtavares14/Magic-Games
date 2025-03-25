#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>

class Buzzer {
public:
    Buzzer(uint8_t buzzerPin);
    void begin();
    void playTone(int frequency, int duration);
    void playErrorTone();
    void playSuccessMelody();
    
private:
    uint8_t buzzerPin;
};

#endif
