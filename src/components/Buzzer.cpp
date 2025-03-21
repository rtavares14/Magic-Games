#include "Buzzer.h"

Buzzer::Buzzer(uint8_t buzzerPin) : buzzerPin(buzzerPin) {}

void Buzzer::begin() {
    pinMode(buzzerPin, OUTPUT);
}

void Buzzer::playTone(int frequency, int duration) {
    tone(buzzerPin, frequency, duration);
    delay(duration);
    noTone(buzzerPin);
}

void Buzzer::playErrorTone() {
    // Example: play a 300 Hz tone for 300ms.
    playTone(300, 300);
}

void Buzzer::playSuccessMelody() {
    int melody[] = {261, 293, 329, 349, 392, 440, 493, 523};
    playTone(melody[0], 400);
    delay(500);
    for (int i = 1; i < 7; i++) {
        playTone(melody[i], 200);
        delay(250);
    }
    playTone(melody[7], 400);
    delay(500);
}
