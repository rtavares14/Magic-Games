// Game1.cpp – Non-blocking version
#include "Game1.h"
#include <Arduino.h>
#include "../components/LCD.h"
#include "../components/KeyLed.h"
#include "../components/Buzzer.h"
#include "../components/RGBLed.h"
#include "../components/Button.h"
#include "../components/Potentiometer.h"
#include "pins.h"

// Declare global objects from main.cpp.
extern LCD lcd;
extern KeyLed keyLed;
extern RGBLed rgb;
extern Buzzer buzzer;
extern Button button;

// Use the global timer defined in main.cpp.
extern uint32_t globalStartTime;
extern const uint32_t TOTAL_TIME;

#define FREQ_SEARCH            500
#define FREQ_CORRECT           1200
#define THRESHOLD              55
#define CONFIRMATION_THRESHOLD 75
#define PRINT_CHANGE_THRESHOLD 80
#define CONFIRMATION_DELAY     80

// Define the internal states for Game1.
enum Game1State {
  GAME1_INIT,
  GAME1_PLAY,
  GAME1_COMPLETE,
  GAME1_TIME_UP
};

enum WaitState { WAIT_FOR_CORRECT_VALUE, CONFIRMING };

bool updateGame1NonBlocking() {
  // Use static variables to hold the game state between calls.
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
  
  // Check global timer expiration.
  uint32_t elapsedGlobal = millis() - globalStartTime;
  if (elapsedGlobal >= TOTAL_TIME) {
    if (gameState != GAME1_TIME_UP) {
      gameState = GAME1_TIME_UP;
      stateStart = millis();
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Time is up!");
      lcd.setCursor(0, 1);
      lcd.print("Vault locked!");
      buzzer.playTone(400, 200);
      rgb.setColor(255, 0, 0);
    }
    return true; // End Game1 (failed)
  }
  
  switch (gameState) {
    case GAME1_INIT: {
      // Show a sequence of messages non-blocking.
      uint32_t t = millis() - stateStart;
      lcd.clear();
      if (t < 2000) {
        lcd.setCursor(0, 0);
        lcd.print("Welcome: LEVEL 1");
        lcd.setCursor(0, 1);
        lcd.print("Get Ready!");
      }
      else if (t < 5000) {
        lcd.setCursor(0, 0);
        lcd.print("Escape in 3 min");
        lcd.setCursor(0, 1);
        lcd.print("or rocks hit you");
      }
      else if (t < 8000) {
        lcd.setCursor(0, 0);
        lcd.print("Listen carefully");
        lcd.setCursor(0, 1);
        lcd.print("find the beep");
      }
      else if (t < 11000) {
        lcd.setCursor(0, 0);
        lcd.print("When found,");
        lcd.setCursor(0, 1);
        lcd.print("press the button");
      }
      else {
        // Initialize game parameters
        randomSeed(analogRead(A1));
        for (int i = 0; i < 3; i++) {
          combo[i] = random(0, 361) * 10;
        }
        Serial.print("Vault combo: ");
        Serial.print(combo[0]); Serial.print(" ");
        Serial.print(combo[1]); Serial.print(" ");
        Serial.println(combo[2]);
        rgb.setColor(255, 0, 255);
        currentStep = 0;
        lastPrintedValue = -100;
        waitState = WAIT_FOR_CORRECT_VALUE;
        comboInitialized = true;
        gameState = GAME1_PLAY;
        stateStart = millis();
      }
      break;
    }
    case GAME1_PLAY: {
      // Update the timer display (if desired; main loop already does this).
      uint32_t elapsed = millis() - globalStartTime;
      keyLed.displayTime(elapsed, TOTAL_TIME, currentStep);
      
      // Read potentiometer.
      Potentiometer potentiometer(PIN_POT);
      int potValue = potentiometer.readValue();
      int currentValue = map(potValue, 0, 1023, 0, 3600);
      if (abs(currentValue - lastPrintedValue) > PRINT_CHANGE_THRESHOLD)
        lastPrintedValue = currentValue;
      
      // Set thresholds based on current step (levels 1 to 3).
      int currentLevel = currentStep + 1;
      int levelThreshold, levelTone;
      switch (currentLevel) {
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
      
      // Tone Generation (non-blocking).
      if (distance < levelThreshold) {
        buzzer.playTone(levelTone, 50);
      } else if (distance < levelThreshold + 5) {
        if (millis() - lastHoverBeepTime > 500) {
          buzzer.playTone(1000, 100);
          lastHoverBeepTime = millis();
        } else {
          int toneFreq = map(distance, 0, 3600, levelTone, FREQ_SEARCH);
          buzzer.playTone(toneFreq, 50);
        }
      } else {
        int toneFreq = map(distance, 0, 3600, levelTone, FREQ_SEARCH);
        buzzer.playTone(toneFreq, 50);
      }
      
      // LED Feedback.
      if (distance < levelThreshold) {
        if (currentLevel == 3)
          rgb.setColor(255, 0, 0);
        else
          rgb.setColor(255, 50, 0);
      } else {
        rgb.setColor(255, 0, 0);
      }
      
      // Process button input.
      button.update();
      bool buttonPressed = button.isPressed();
      if (waitState == WAIT_FOR_CORRECT_VALUE) {
        if (distance < levelThreshold && buttonPressed) {
          confirmStartTime = millis();
          waitState = CONFIRMING;
        }
      } else if (waitState == CONFIRMING) {
        if (distance >= CONFIRMATION_THRESHOLD)
          waitState = WAIT_FOR_CORRECT_VALUE;
        else if (millis() - confirmStartTime >= CONFIRMATION_DELAY) {
          currentStep++;
          Serial.print("Step ");
          Serial.print(currentStep);
          Serial.println(" confirmed!");
          if (currentStep < 3) {
            char stepMsg[17];
            sprintf(stepMsg, "STEP %d OF 3 DONE", currentStep);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(stepMsg);
            lcd.setCursor(0, 1);
            lcd.print("KEEP GOING");
          } else {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Vault opened!");
            lcd.setCursor(0, 1);
            lcd.print("Congrats!");
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
    case GAME1_COMPLETE: {
      // Hold success message for 2 seconds then finish.
      if (millis() - stateStart > 2000)
        return true;
      break;
    }
    case GAME1_TIME_UP:
      return true;
  }
  
  return false; // Game1 is still running.
}
