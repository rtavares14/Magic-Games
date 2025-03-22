// Game2.cpp – Non-blocking version using non-blocking LCD printing
#include "Game2.h"
#include <Arduino.h>
#include "../components/LCD.h"
#include "../components/KeyLed.h"
#include "../components/Buzzer.h"
#include "../components/RGBLed.h"
#include "../components/Button.h"
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

bool updateGame2NonBlocking() {
  static Game2State gameState = GAME2_INIT;
  static unsigned long stateStart = millis();
  static int attemptCount = 0;
  static String userInput = "";
  static unsigned long lastKeyPressTime = 0;
  static uint8_t lastButtons = 0;
  static unsigned long game2LocalStart = 0;
  
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
      } else {
         gameState = GAME2_PLAY;
         stateStart = millis();
         lastMsgIndex = -1;
         break;
      }
      if (msgIndex != lastMsgIndex) {
         lcd.clear();
         switch(msgIndex) {
            case 0:
              lcd.setCursor(0, 0);
              lcd.print("Welcome: LEVEL 2");
              lcd.setCursor(0, 1);
              lcd.print("Find the tune!");
              rgb.setColor(0, 0, 255);
              break;
         }
         lastMsgIndex = msgIndex;
      }
      break;
    }
    case GAME2_PLAY: {
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
          // Use a static variable to update tip message only when it changes.
          static String lastTip = "";
          char tipHeader[17];
          sprintf(tipHeader, "Tip for note %d", (int)userInput.length() + 1);
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
      }
      lastButtons = keys;
      
      button.update();
      if (button.isPressed() && userInput.length() > 0) {
          String compareInput = userInput.substring(0, min((int)userInput.length(), MELODY_LENGTH));
          if (compareInput.equals(targetMelody)) {
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
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Wrong Tune!");
              lcd.setCursor(0, 1);
              lcd.print("Try again!");
              rgb.setColor(255, 0, 0);
              buzzer.playErrorTone();
              userInput = "";
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
