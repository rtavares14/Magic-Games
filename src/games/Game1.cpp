#include "Game1.h"
#include <Arduino.h>
#include "LCD.h"
#include "KeyLed.h"
#include "Buzzer.h"
#include "RGBLed.h"
#include "Button.h"
#include "Potentiometer.h"
#include "pins.h"
#include "Score.h"

// Declare global objects from main.cpp.
extern LCD lcd;
extern KeyLed keyLed;
extern RGBLed rgb;
extern Buzzer buzzer;
extern Button button;

// Use the global timer defined in main.cpp.
extern uint32_t globalStartTime;
extern const uint32_t TOTAL_TIME;

// Declare the global game press counter.
extern int currentGamePresses;
extern int game1FinalScore;

#define FREQ_SEARCH 500
#define FREQ_CORRECT 1200
#define THRESHOLD 55
#define CONFIRMATION_THRESHOLD 75
#define PRINT_CHANGE_THRESHOLD 80
#define CONFIRMATION_DELAY 80

// Define the internal states for Game1.
enum Game1State
{
  GAME1_INIT,
  GAME1_PLAY,
  GAME1_COMPLETE,
  GAME1_TIME_UP
};

enum WaitState
{
  WAIT_FOR_CORRECT_VALUE,
  CONFIRMING
};

bool updateGame1()
{
  static Game1State gameState = GAME1_INIT;
  static unsigned long stateStart = millis();
  static int currentStep = 0;
  static int combo[3];
  static bool comboInitialized = false;
  static int lastPrintedValue = -100;
  static WaitState waitState = WAIT_FOR_CORRECT_VALUE;
  static unsigned long confirmStartTime = 0;
  static unsigned long lastHoverBeepTime = 0;
  static bool safeOpened = false;
  // New variable to record the start time of Game1.
  static uint32_t game1StartTime = 0;

  // Check global timer expiration.
  uint32_t elapsedGlobal = millis() - globalStartTime;
  if (elapsedGlobal >= TOTAL_TIME)
  {
    if (gameState != GAME1_TIME_UP)
    {
      gameState = GAME1_TIME_UP;
      stateStart = millis();
      lcd.lcdShow("Time is up!", "Vault locked!");
      buzzer.playTone(400, 200);
      rgb.setColor(255, 0, 0);
    }
    return true; // End Game1 (failed)
  }

  switch (gameState)
  {
  case GAME1_INIT:
  {
    uint32_t t = millis() - stateStart;
    static int lastMsgIndex = -1;
    int msgIndex = -1;
    if (t < 2000)
    {
      msgIndex = 0;
    }
    else if (t < 5000)
    {
      msgIndex = 1;
    }
    else if (t < 8000)
    {
      msgIndex = 2;
    }
    else if (t < 11000)
    {
      msgIndex = 3;
    }
    else if (t < 14000)
    {
      msgIndex = 4;
    }
    else
    {
      randomSeed(analogRead(A1));
      for (int i = 0; i < 3; i++)
      {
        combo[i] = random(3, 33) * 100;
      }
      Serial.println("------------------------------------");
      Serial.println("---------------Game-1---------------");
      Serial.println("Vault combo generated!");
      Serial.print("Vault combo: ");
      Serial.print(combo[0]);
      Serial.print(" ");
      Serial.print(combo[1]);
      Serial.print(" ");
      Serial.println(combo[2]);
      rgb.setColor(255, 0, 255);
      currentStep = 0;
      lastPrintedValue = -100;
      waitState = WAIT_FOR_CORRECT_VALUE;
      comboInitialized = true;
      gameState = GAME1_PLAY;
      game1StartTime = millis();
      stateStart = millis();
      lastMsgIndex = -1;
      break;
    }

    if (msgIndex != lastMsgIndex)
    {
      lcd.lcdShow(
          (msgIndex == 0) ? "Welcome: LEVEL 1" :
          (msgIndex == 1) ? "Escape fast or" :
          (msgIndex == 2) ? "Listen carefully" :
          (msgIndex == 3) ? "When found, you" :
                            "Dont forget to",
          (msgIndex == 0) ? "Get Ready!" :
          (msgIndex == 1) ? "rocks hit you" :
          (msgIndex == 2) ? "find the beep" :
          (msgIndex == 3) ? "are close!" :
                            "press the button");
      lastMsgIndex = msgIndex;
    }
    break;
  }
  case GAME1_PLAY:
  {
    uint32_t elapsed = millis() - globalStartTime;
    keyLed.displayTime(elapsed, TOTAL_TIME, currentGamePresses);

    // Read potentiometer.
    Potentiometer potentiometer(PIN_POT);
    int potValue = potentiometer.readValue();
    int currentValue = map(potValue, 0, 1023, 0, 3600);
    if (abs(currentValue - lastPrintedValue) > PRINT_CHANGE_THRESHOLD)
      lastPrintedValue = currentValue;

    // Set thresholds based on current step.
    int currentLevel = currentStep + 1;
    int levelThreshold, levelTone;
    switch (currentLevel)
    {
    case 1:
      levelThreshold = THRESHOLD;
      levelTone = FREQ_CORRECT;
      break;
    case 2:
      levelThreshold = THRESHOLD - 10;
      levelTone = 1500;
      break;
    case 3:
      levelThreshold = THRESHOLD - 20;
      levelTone = 1800;
      break;
    default:
      levelThreshold = THRESHOLD;
      levelTone = FREQ_CORRECT;
      break;
    }

    int target = combo[currentStep];
    int distance = abs(currentValue - target);

    // Tone generation based on distance.
    if (distance < levelThreshold)
    {
      buzzer.playTone(levelTone, 50);
    }
    else if (distance < levelThreshold + 5)
    {
      if (millis() - lastHoverBeepTime > 500)
      {
        buzzer.playTone(1000, 100);
        lastHoverBeepTime = millis();
      }
      else
      {
        int toneFreq = map(distance, 0, 3600, levelTone, FREQ_SEARCH);
        buzzer.playTone(toneFreq, 50);
      }
    }
    else
    {
      int toneFreq = map(distance, 0, 3600, levelTone, FREQ_SEARCH);
      buzzer.playTone(toneFreq, 50);
    }

    // LED feedback.
    if (distance < levelThreshold)
    {
      if (currentLevel == 3)
        rgb.setColor(255, 0, 0);
      else
        rgb.setColor(255, 50, 0);
    }
    else
    {
      rgb.setColor(255, 0, 0);
    }

    // Process button input.
    bool buttonPressed = button.isPressed();
    if (buttonPressed)
    {
      Serial.print("Button pressed with value: ");
      Serial.println(currentValue);
    }
    if (waitState == WAIT_FOR_CORRECT_VALUE)
    {
      if (distance < levelThreshold && buttonPressed)
      {
        confirmStartTime = millis();
        waitState = CONFIRMING;
      }
    }
    else if (waitState == CONFIRMING)
    {
      if (distance >= CONFIRMATION_THRESHOLD)
        waitState = WAIT_FOR_CORRECT_VALUE;
      else if (millis() - confirmStartTime >= CONFIRMATION_DELAY)
      {
        currentStep++;
        Serial.print("Step ");
        Serial.print(currentStep);
        Serial.println(" confirmed!");
        if (currentStep < 3)
        {
          char stepMsg[17];
          sprintf(stepMsg, "STEP %d OF 3 DONE", currentStep);
          lcd.lcdShow(stepMsg, "KEEP GOING");
        }
        else
        {
          lcd.lcdShow("Vault opened!", "Congrats!");
          Serial.println("Vault opened!");
          keyLed.printTimeUsed(game1StartTime);
          Serial.println("Button presses: " + String(currentGamePresses));
          unsigned long timeTaken = millis() - game1StartTime;
          Score game1Score(1, currentGamePresses, timeTaken);
          game1FinalScore = game1Score.points;
          Serial.print("Game 1 Score: ");
          Serial.println(game1Score.points);
          rgb.setColor(0, 255, 0);
          buzzer.playSuccessMelody();
          safeOpened = true;
          gameState = GAME1_COMPLETE;
          stateStart = millis();
        }
        waitState = WAIT_FOR_CORRECT_VALUE;
        lastPrintedValue = -100;
      }
    }
    break;
  }
  case GAME1_COMPLETE:
  {
    if (millis() - stateStart > 2000)
      return true;
    break;
  }
  case GAME1_TIME_UP:
    return true;
  }

  return false;
}
