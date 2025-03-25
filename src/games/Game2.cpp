#include "Game2.h"
#include <Arduino.h>
#include "LCD.h"
#include "KeyLed.h"
#include "Buzzer.h"
#include "RGBLed.h"
#include "Button.h"
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
extern int currentGamePresses;  // From main.cpp

const int MELODY_LENGTH = 8;
const int KEY_DEBOUNCE_DELAY = 150;
const unsigned long BUTTON_DEBOUNCE_DELAY = 50;

String targetMelody = "48215637";
String tips[MELODY_LENGTH] = {
  "A quartet awaits",    
  "Infinite curve",      
  "A pair in tune",      
  "The lone melody",      
  "Middle of magic",      
  "Sixth sense",          
  "Triple allure",        
  "Lucky final touch"     
};

char keyDigits[8] = {'1','2','3','4','5','6','7','8'};
int noteFrequencies[8] = {261, 293, 329, 349, 392, 440, 493, 523};

enum Game2State {
  GAME2_INIT,
  GAME2_PLAY,
  GAME2_COMPLETE,
  GAME2_TIME_UP
};

bool updateGame2() {
  static Game2State gameState = GAME2_INIT;
  static unsigned long stateStart = millis();
  static int attemptCount = 0;
  static String userInput = "";
  static unsigned long lastKeyPressTime = 0;
  static uint8_t lastButtons = 0;
  // New variable to mark the start time of Game 2.
  static uint32_t game2StartTime = 0;

  // Check global timer expiration.
  uint32_t elapsedGlobal = millis() - globalStartTime;
  if (elapsedGlobal >= TOTAL_TIME) {
    if (gameState != GAME2_TIME_UP) {
      gameState = GAME2_TIME_UP;
      stateStart = millis();
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Time's up!");
      lcd.setCursor(0, 1);
      lcd.print("Game Over!");
      buzzer.playTone(400, 200);
      rgb.setColor(255, 0, 0);
    }
    return true;
  }
  
  switch (gameState) {
    case GAME2_INIT: {
      uint32_t t = millis() - stateStart;
      static int lastMsgIndex = -1;
      int msgIndex = -1;
      if (t < 2000) {
         msgIndex = 0;
         if (msgIndex != lastMsgIndex) {
           lcd.clear();
           lcd.setCursor(0, 0);
           lcd.print("Welcome: LEVEL 2");
           lcd.setCursor(0, 1);
           lcd.print("Find the tune!");
           rgb.setColor(0, 0, 255);
           lastMsgIndex = msgIndex;
         }
      } else {
         // After 2 seconds, clear the welcome message and display the first tip.
         lcd.clear();
         char tipHeader[17];
         sprintf(tipHeader, "Tip for note %d", 1);
         lcd.setCursor(0, 0);
         lcd.print(tipHeader);
         lcd.setCursor(0, 1);
         String tip = tips[0];
         if (tip.length() > 16)
             tip = tip.substring(0, 16);
         lcd.print(tip.c_str());
         // Print debug header for Game2.
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
    case GAME2_PLAY: {
      static bool finalMessageDisplayed = false;
      static String lastTip = "";
      uint8_t keys = keyLed.readButtons();
      int pressedCount = 0;
      int pressedIndex = -1;
      for (int i = 0; i < 8; i++) {
          bool pressed = (keys & (1 << i)) != 0;
          if (pressed) {
              pressedCount++;
              if (pressedIndex == -1)
                  pressedIndex = i;
              keyLed.setLED(i, true);
          } else {
              keyLed.setLED(i, false);
          }
      }
      
      if (pressedCount == 1 &&
          (millis() - lastKeyPressTime > KEY_DEBOUNCE_DELAY) &&
          ((lastButtons & (1 << pressedIndex)) == 0)) {
          userInput += keyDigits[pressedIndex];
          buzzer.playTone(noteFrequencies[pressedIndex], 200);
          lastKeyPressTime = millis();
          char tipHeader[17];
          sprintf(tipHeader, "Tip for note %d", (int)userInput.length() + 1);
          if (userInput.length() < MELODY_LENGTH) {
              String tip = tips[userInput.length()];
              if (tip.length() > 16)
                  tip = tip.substring(0, 16);
              if (tip != lastTip) {
                  lcd.clear();
                  lcd.setCursor(0, 0);
                  lcd.print(tipHeader);
                  lcd.setCursor(0, 1);
                  lcd.print(tip.c_str());
                  lastTip = tip;
              }
          } else if (userInput.length() == MELODY_LENGTH && !finalMessageDisplayed) {
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Maybe give a ,");
              lcd.setCursor(0, 1);
              lcd.print("try to the melody");
              finalMessageDisplayed = true;
          }
      }
      lastButtons = keys;
      
      if (button.isPressed() && userInput.length() > 0) {
          // Compute how many consecutive digits match the target melody.
          int correctCount = 0;
          for (int i = 0; i < userInput.length() && i < targetMelody.length(); i++) {
              if (userInput.charAt(i) == targetMelody.charAt(i))
                  correctCount++;
              else
                  break;
          }
          
          // If the complete melody is correct.
          if (userInput.equals(targetMelody)) {
              Serial.print("Try number ");
              Serial.print(attemptCount + 1);
              Serial.println(": melody correct");
              // Print time used in Game 2 (measured from game2StartTime)
              keyLed.printTimeUsed(game2StartTime);
              Serial.print("Button presses: ");
              Serial.println(currentGamePresses);
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Correct Tune!");
              lcd.setCursor(0, 1);
              lcd.print("Well done!");
              rgb.setColor(0, 255, 0);
              buzzer.playSuccessMelody();
              gameState = GAME2_COMPLETE;
              stateStart = millis();
          } else {
              attemptCount++;
              Serial.print("Try number ");
              Serial.print(attemptCount);
              Serial.print(": ");
              Serial.print(correctCount);
              Serial.println(" correct");
              // Deduct 30 seconds from the global timer.
              globalStartTime -= 30000;
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Wrong Tune!");
              lcd.setCursor(0, 1);
              lcd.print("Try again!");
              rgb.setColor(255, 0, 0);
              buzzer.playErrorTone();
              userInput = "";
              // Reset final message flag and tip cache.
              finalMessageDisplayed = false;
              lastTip = "";
              // Display the first tip again.
              lcd.clear();
              char tipHeader[17];
              sprintf(tipHeader, "Tip for note %d", 1);
              lcd.setCursor(0, 0);
              lcd.print(tipHeader);
              lcd.setCursor(0, 1);
              String tip = tips[0];
              if (tip.length() > 16)
                  tip = tip.substring(0, 16);
              lcd.print(tip.c_str());
          }
      }
      break;
    }
    case GAME2_COMPLETE: {
      if (millis() - stateStart > 2000)
          return true;
      break;
    }
    case GAME2_TIME_UP:
      return true;
  }
  
  return false; // Game2 is still running.
}
