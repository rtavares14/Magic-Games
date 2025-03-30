#include "Buzzer.h"

Buzzer::Buzzer(uint8_t buzzerPin) : buzzerPin(buzzerPin) {}

void Buzzer::begin()
{
  pinMode(buzzerPin, OUTPUT);
}

void Buzzer::playTone(int frequency, int duration)
{
  tone(buzzerPin, frequency, duration);
  delay(duration);
  noTone(buzzerPin);
}

void Buzzer::playErrorTone()
{
  playTone(300, 200);
}

void Buzzer::playSuccessMelody()
{
  int melody[] = {261, 293, 329, 349, 392, 440, 493, 523};
  playTone(melody[0], 200);
  delay(200);
  for (int i = 1; i < 7; i++)
  {
    playTone(melody[i], 100);
    delay(150);
  }
  playTone(melody[7], 200);
  delay(200);
}

void Buzzer::playGameOverMelody()
{
  int melody[] = {523, 493, 440, 392, 349, 329, 293, 261};
  for (int i = 0; i < 8; i++)
  {
    playTone(melody[i], 100);
    delay(50);
  }
}

void Buzzer::playWinningMelody()
{
  int melody[] = {261, 293, 329, 349, 392, 440, 493, 523};
  for (int i = 0; i < 8; i++)
  {
    playTone(melody[i], 100);
    delay(50);
  }
}
