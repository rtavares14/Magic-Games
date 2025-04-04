#include "Game2.h"
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

// Declare global objects from main.cpp.
extern LCD lcd;
extern KeyLed keyLed;
extern RGBLed rgb;
extern Buzzer buzzer;
extern Button button;

// Use the global timer defined in main.cpp.
extern uint32_t globalStartTime;
extern const uint32_t TOTAL_TIME;
extern int currentGamePresses;
extern int game2FinalScore;

//-----------------------
// Constant Definitions
//-----------------------

// Game parameters
const int MELODY_LENGTH = 8;
const int KEY_DEBOUNCE_DELAY = 150;
const unsigned long BUTTON_DEBOUNCE_DELAY = 50; // Unused but defined

// Time durations (in milliseconds)
const unsigned long GAME2_INIT_DURATION = 2000;
const unsigned long TONE_NOTE_DURATION = 200;
const unsigned long GAME2_WRONG_DURATION = 1000;
const unsigned long GAME2_COMPLETE_DURATION = 2000;
const unsigned long PENALTY_TIME_INCREMENT = 30000;

// Color definitions
const int COLOR_BLUE_R = 0;
const int COLOR_BLUE_G = 0;
const int COLOR_BLUE_B = 255;

const int COLOR_RED_R = 255;
const int COLOR_RED_G = 0;
const int COLOR_RED_B = 0;

const int COLOR_GREEN_R = 0;
const int COLOR_GREEN_G = 255;
const int COLOR_GREEN_B = 0;

// Melody and Tips
//-----------------------
String targetMelody = "48215637";

String tips[MELODY_LENGTH] = {
    "A quartet awaits",
    "Infinite curve",
    "A pair in tune",
    "The lone melody",
    "Middle of magic",
    "Sixth sense",
    "Triple allure",
    "Lucky final touch"};

char keyDigits[MELODY_LENGTH] = {'1', '2', '3', '4', '5', '6', '7', '8'};
int noteFrequencies[MELODY_LENGTH] = {261, 293, 329, 349, 392, 440, 493, 523};

// Game State Definitions
enum Game2State
{
  GAME2_INIT,
  GAME2_PLAY,
  GAME2_WRONG,
  GAME2_COMPLETE,
  GAME2_TIME_UP
};

bool updateGame2()
{
  static Game2State gameState = GAME2_INIT;
  static unsigned long stateStart = millis();
  static int attemptCount = 0;
  static unsigned long penaltyTime = 0; // Accumulate penalty time here
  static String userInput = "";
  static unsigned long lastKeyPressTime = 0;
  static uint8_t lastButtons = 0;
  // Record the start time of Game 2.
  static uint32_t game2StartTime = 0;

  switch (gameState)
  {
  case GAME2_INIT:
  {
    uint32_t t = millis() - stateStart;
    static int lastMsgIndex = -1;
    int msgIndex = -1;
    if (t < GAME2_INIT_DURATION)
    {
      msgIndex = 0;
      if (msgIndex != lastMsgIndex)
      {
        lcd.lcdShow("Welcome: LEVEL 2", "Find the tune!");
        rgb.setColor(COLOR_BLUE_R, COLOR_BLUE_G, COLOR_BLUE_B);
        lastMsgIndex = msgIndex;
      }
    }
    else
    {
      // After init duration, display the first tip.
      char tipHeader[17];
      sprintf(tipHeader, "Tip for note %d", 1);
      lcd.lcdShow(tipHeader, tips[0].substring(0, 16).c_str());
      Serial.println("------------------------------------");
      Serial.println("---------------Game-2---------------");
      Serial.println("Melody generated!");
      Serial.print("Melody: ");
      Serial.println(targetMelody);
      // Set the start time for Game 2.
      game2StartTime = millis();
      gameState = GAME2_PLAY;
      stateStart = millis();
      lastMsgIndex = -1;
      break;
    }
    break;
  }
  case GAME2_PLAY:
  {
    static bool finalMessageDisplayed = false;
    static String lastTip = "";
    uint8_t keys = keyLed.readButtons();
    int pressedCount = 0;
    int pressedIndex = -1;
    for (int i = 0; i < MELODY_LENGTH; i++)
    {
      bool pressed = (keys & (1 << i)) != 0;
      if (pressed)
      {
        pressedCount++;
        if (pressedIndex == -1)
          pressedIndex = i;
        keyLed.setLED(i, true);
      }
      else
      {
        keyLed.setLED(i, false);
      }
    }

    if (pressedCount == 1 &&
        (millis() - lastKeyPressTime > KEY_DEBOUNCE_DELAY) &&
        ((lastButtons & (1 << pressedIndex)) == 0))
    {
      userInput += keyDigits[pressedIndex];
      buzzer.playTone(noteFrequencies[pressedIndex], TONE_NOTE_DURATION);
      lastKeyPressTime = millis();
      char tipHeader[17];
      sprintf(tipHeader, "Tip for note %d", (int)userInput.length() + 1);
      if (userInput.length() < MELODY_LENGTH)
      {
        String tip = tips[userInput.length()];
        if (tip.length() > 16)
          tip = tip.substring(0, 16);
        if (tip != lastTip)
        {
          lcd.lcdShow(tipHeader, tip.c_str());
          lastTip = tip;
        }
      }
      else if (userInput.length() == MELODY_LENGTH && !finalMessageDisplayed)
      {
        lcd.lcdShow("Maybe give a ,", "try to the melody");
        finalMessageDisplayed = true;
      }
    }
    lastButtons = keys;

    if (button.isPressed() && userInput.length() > 0)
    {
      // Debug: print current user input.
      Serial.print("User submitted melody: ");
      Serial.println(userInput);

      int correctCount = 0;
      for (int i = 0; i < userInput.length() && i < targetMelody.length(); i++)
      {
        if (userInput.charAt(i) == targetMelody.charAt(i))
          correctCount++;
        else
          break;
      }

      if (userInput.equals(targetMelody))
      {
        Serial.print("Try number ");
        Serial.print(attemptCount + 1);
        Serial.println(": melody correct");
        keyLed.printTimeUsed(game2StartTime);
        Serial.print("Button presses: ");
        Serial.println(currentGamePresses);

        // Compute effective time as elapsed time plus the accumulated penalty.
        unsigned long effectiveTime = (millis() - game2StartTime) + penaltyTime;

        unsigned long totalSeconds = effectiveTime / 1000;
        unsigned long minutes = totalSeconds / 60;
        unsigned long seconds = totalSeconds % 60;
        Serial.print("Time used: ");
        Serial.print(minutes);
        Serial.print("m and ");
        Serial.print(seconds);
        Serial.println("s");

        lcd.lcdShow("Correct Tune!", "Well done!");
        Score game2Score(2, currentGamePresses, effectiveTime);
        game2FinalScore = game2Score.points;
        Serial.print("Game 2 Score: ");
        Serial.println(game2Score.points);
        rgb.setColor(COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B);
        buzzer.playSuccessMelody();
        gameState = GAME2_COMPLETE;
        stateStart = millis();
      }
      else
      {
        attemptCount++;
        penaltyTime += PENALTY_TIME_INCREMENT;
        globalStartTime -= PENALTY_TIME_INCREMENT;
        Serial.print("Try number ");
        Serial.print(attemptCount);
        Serial.print(": ");
        Serial.print(correctCount);
        Serial.println(" correct");
        lcd.lcdShow("Wrong Tune!", "Try again!");
        rgb.setColor(COLOR_RED_R, COLOR_RED_G, COLOR_RED_B);
        buzzer.playErrorTone();
        gameState = GAME2_WRONG;
        stateStart = millis();
      }
    }
    break;
  }
  case GAME2_WRONG:
  {
    if (millis() - stateStart >= GAME2_WRONG_DURATION)
    {
      rgb.setColor(COLOR_BLUE_R, COLOR_BLUE_G, COLOR_BLUE_B);
      userInput = "";
      lcd.lcdShow("Tip for note 1", tips[0].substring(0, 16).c_str());
      gameState = GAME2_PLAY;
      stateStart = millis();
    }
    break;
  }
  case GAME2_COMPLETE:
  {
    if (millis() - stateStart > GAME2_COMPLETE_DURATION)
      return true;
    break;
  }
  case GAME2_TIME_UP:
    return true;
  }
  return false; // Game2 is still running.
}
