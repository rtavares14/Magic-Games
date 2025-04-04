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
#include "Globals.h"

// Constant Definitions

// Number of levels in the game
const int NUM_LEVELS = 3;

// Tone frequency and threshold constants
const int TUNE_SEARCH = 500;                    // Frequency for distant tone feedback
const int TUNE_CORRECT = 1200;                  // Frequency for correct guess tone
const int THRESHOLD_LEVEL1 = 65;                // Base threshold for level 1
const int CONFIRMATION_THRESHOLD = 75;          // Threshold to cancel confirmation
const int PRINT_CHANGE_THRESHOLD = 80;          // Minimum change needed to update printed value
const unsigned long CONFIRMATION_DELAY_MS = 80; // Delay required for confirmation (ms)

// Tone for time-up event
const int TONE_TIME_UP_FREQUENCY = 400;          // Frequency when time is up
const unsigned long TONE_TIME_UP_DURATION = 200; // Duration when time is up (ms)

// Message timing constants (in milliseconds)
const unsigned long MSG_STAGE0 = 2000;
const unsigned long MSG_STAGE1 = 5000;
const unsigned long MSG_STAGE2 = 8000;
const unsigned long MSG_STAGE3 = 11000;
const unsigned long MSG_STAGE4 = 14000;

// Random combo generation constants
const int RANDOM_MIN = 5;
const int RANDOM_MAX = 32;
const int RANDOM_MULTIPLIER = 100;

// Level-specific adjustments for thresholds and tones
const int LEVEL2_THRESHOLD_OFFSET = 10;
const int LEVEL3_THRESHOLD_OFFSET = 20;
const int TUNE_LEVEL2 = 1500;
const int TUNE_LEVEL3 = 1800;

// Tone duration and interval constants (in milliseconds)
const unsigned long TONE_DURATION_SHORT = 50;
const unsigned long TONE_DURATION_LONG = 100;
const unsigned long HOVER_BEEP_INTERVAL = 500;

// Game completion display time
const unsigned long GAME_COMPLETE_DISPLAY_TIME = 2000;

// Game State Definitions

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
  static int combo[NUM_LEVELS];
  static bool comboInitialized = false;
  static int lastPrintedValue = -100;
  static WaitState waitState = WAIT_FOR_CORRECT_VALUE;
  static unsigned long confirmStartTime = 0;
  static unsigned long lastHoverBeepTime = 0;
  static bool safeOpened = false;
  // Record the start time of Game1.
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
      buzzer.playTone(TONE_TIME_UP_FREQUENCY, TONE_TIME_UP_DURATION);
      rgb.setColor(255, 0, 0);
    }
    return true; // End Game1
  }

  switch (gameState)
  {
  case GAME1_INIT:
  {
    uint32_t t = millis() - stateStart;
    static int lastMsgIndex = -1;
    int msgIndex = -1;
    if (t < MSG_STAGE0)
    {
      msgIndex = 0;
    }
    else if (t < MSG_STAGE1)
    {
      msgIndex = 1;
    }
    else if (t < MSG_STAGE2)
    {
      msgIndex = 2;
    }
    else if (t < MSG_STAGE3)
    {
      msgIndex = 3;
    }
    else if (t < MSG_STAGE4)
    {
      msgIndex = 4;
    }
    else
    {
      randomSeed(analogRead(A1));
      for (int i = 0; i < NUM_LEVELS; i++)
      {
        combo[i] = random(RANDOM_MIN, RANDOM_MAX) * RANDOM_MULTIPLIER;
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
          (msgIndex == 0) ? "Welcome: LEVEL 1" : (msgIndex == 1) ? "Escape fast or"
                                             : (msgIndex == 2)   ? "Listen carefully"
                                             : (msgIndex == 3)   ? "When found, you"
                                                                 : "Dont forget to",
          (msgIndex == 0) ? "Get Ready!" : (msgIndex == 1) ? "rocks hit you"
                                       : (msgIndex == 2)   ? "find the beep"
                                       : (msgIndex == 3)   ? "are close!"
                                                           : "press the button");
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

    // Set thresholds and tone frequencies based on current step.
    int currentLevel = currentStep + 1;
    int levelThreshold, levelTone;
    switch (currentLevel)
    {
    case 1:
      levelThreshold = THRESHOLD_LEVEL1;
      levelTone = TUNE_CORRECT;
      break;
    case 2:
      levelThreshold = THRESHOLD_LEVEL1 - LEVEL2_THRESHOLD_OFFSET;
      levelTone = TUNE_LEVEL2;
      break;
    case NUM_LEVELS:
      levelThreshold = THRESHOLD_LEVEL1 - LEVEL3_THRESHOLD_OFFSET;
      levelTone = TUNE_LEVEL3;
      break;
    default:
      levelThreshold = THRESHOLD_LEVEL1;
      levelTone = TUNE_CORRECT;
      break;
    }

    int target = combo[currentStep];
    int distance = abs(currentValue - target);

    // Tone generation based on distance.
    if (distance < levelThreshold)
    {
      buzzer.playTone(levelTone, TONE_DURATION_SHORT);
    }
    else if (distance < levelThreshold + 5)
    {
      if (millis() - lastHoverBeepTime > HOVER_BEEP_INTERVAL)
      {
        buzzer.playTone(1000, TONE_DURATION_LONG);
        lastHoverBeepTime = millis();
      }
      else
      {
        int toneFreq = map(distance, 0, 3600, levelTone, TUNE_SEARCH);
        buzzer.playTone(toneFreq, TONE_DURATION_SHORT);
      }
    }
    else
    {
      int toneFreq = map(distance, 0, 3600, levelTone, TUNE_SEARCH);
      buzzer.playTone(toneFreq, TONE_DURATION_SHORT);
    }

    // LED feedback.
    if (distance < levelThreshold)
    {
      if (currentLevel == NUM_LEVELS)
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
      else if (millis() - confirmStartTime >= CONFIRMATION_DELAY_MS)
      {
        currentStep++;
        Serial.print("Step ");
        Serial.print(currentStep);
        Serial.println(" confirmed!");
        if (currentStep < NUM_LEVELS)
        {
          char stepMsg[17];
          sprintf(stepMsg, "STEP %d OF %d DONE", currentStep, NUM_LEVELS);
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
    if (millis() - stateStart > GAME_COMPLETE_DISPLAY_TIME)
      return true;
    break;
  }
  case GAME1_TIME_UP:
    return true;
  }

  return false;
}
