#include "Game1.h"
#include <Arduino.h>
#include "../components/LCD.h"
#include "../components/KeyLed.h"
#include "../components/Buzzer.h"
#include "../components/RGBLed.h"
#include "../components/Button.h"
#include "../components/Potentiometer.h"
#include "pins.h"   // Unchanged pins definitions

// --- Game Constants ---
extern uint32_t gameDuration;
#define FREQ_SEARCH            500
#define FREQ_CORRECT           1200        // Tone for level 1 (normal)
#define THRESHOLD              55          // Base threshold for tone/LED feedback
#define CONFIRMATION_THRESHOLD 75          // Margin to confirm correct value
#define PRINT_CHANGE_THRESHOLD 80          // Minimum change to update printed value
#define CONFIRMATION_DELAY     80          // Confirmation delay in ms

// --- Game State Variables ---
static uint32_t gameStartTime;
static int combo[3];
static int currentStep = 0;
static int lastPrintedValue = -100;  // To force initial print
enum State { WAIT_FOR_CORRECT_VALUE, CONFIRMING };
static State state = WAIT_FOR_CORRECT_VALUE;
static unsigned long confirmStartTime = 0;
static bool safeOpened = false;
static unsigned long lastHoverBeepTime = 0; // For hover beep timing

void runGame1() {
  // --- Instantiate Components ---
  LCD lcd(0x27, 16, 2);                     // LCD at I2C address 0x27
  KeyLed keyLed(STB_PIN, CLK_PIN, DIO_PIN);  // TM1638-based key/LED display
  Buzzer buzzer(PIN_BUZZER);
  RGBLed rgbLed;                           // Uses PIN_RED, PIN_GREEN, PIN_BLUE from pins.h
  Button button(PIN_BUTTON, 50);           // 50ms debounce delay
  Potentiometer potentiometer(PIN_POT);
  
  // --- Initialize Components ---
  lcd.begin();
  keyLed.begin();
  buzzer.begin();
  rgbLed.begin();
  button.begin();
  
  // --- Display Initial Instructions via LCD ---
  lcd.printMessage("Escape in 3 min", "or rocks hit you", 3000);
  lcd.printMessage("Listen carefully", "find the beep", 3000);
  lcd.printMessage("When you find it,", "press the button", 3000);
  
  // Set initial LED color (red/purple) for feedback.
  rgbLed.setColor(255, 0, 255);
  
  // Initialize timer display.
  keyLed.displayTime(0, gameDuration, currentStep);
  
  // Set game start time.
  gameStartTime = millis();
  
  // Randomize vault combination (3 values between 0 and 3600, in steps of 10).
  randomSeed(analogRead(A1));
  for (int i = 0; i < 3; i++) {
    combo[i] = random(0, 361) * 10;
  }
  Serial.print("Vault combo: ");
  Serial.print(combo[0]); Serial.print(" ");
  Serial.print(combo[1]); Serial.print(" ");
  Serial.println(combo[2]);
  
  // --- Main Game Loop ---
  while (true) {
    unsigned long elapsed = millis() - gameStartTime;
    
    // --- Timer Check ---
    if (!safeOpened) {
      if (elapsed >= gameDuration) {
        // Time's up.
        keyLed.displayTime(gameDuration, gameDuration, currentStep);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Time is up!");
        lcd.setCursor(0, 1);
        lcd.print("Vault locked!");
        // Losing feedback: blink red LED and play a low tone repeatedly.
        while (true) {
          buzzer.playTone(400, 200);
          rgbLed.setColor(255, 0, 0);
          delay(200);
          rgbLed.setColor(0, 0, 0);
          delay(300);
        }
      }
      // Update KeyLed display with elapsed time.
      keyLed.displayTime(elapsed, gameDuration, currentStep);
    }
    
    // --- Game Logic ---
    if (!safeOpened && currentStep < 3) {
      int currentLevel = currentStep + 1;
      int levelThreshold, levelTone;
      // Adjust threshold and tone based on game level.
      switch(currentLevel) {
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
      
      // Read potentiometer and map the value.
      int potValue = potentiometer.readValue();
      int currentValue = map(potValue, 0, 1023, 0, 3600);
      if (abs(currentValue - lastPrintedValue) > PRINT_CHANGE_THRESHOLD) {
        lastPrintedValue = currentValue;
      }
      
      int target = combo[currentStep];
      int distance = abs(currentValue - target);
      
      // --- Tone Generation (Hover Beep) via Buzzer ---
      if (distance < levelThreshold) {
        buzzer.playTone(levelTone, 50);
      }
      else if (distance < levelThreshold + 5) {
        if (millis() - lastHoverBeepTime > 500) {
          buzzer.playTone(1000, 100); // Short beep for hover feedback.
          lastHoverBeepTime = millis();
        } else {
          int toneFreq = map(distance, 0, 3600, levelTone, FREQ_SEARCH);
          buzzer.playTone(toneFreq, 50);
        }
      }
      else {
        int toneFreq = map(distance, 0, 3600, levelTone, FREQ_SEARCH);
        buzzer.playTone(toneFreq, 50);
      }
      
      // --- LED Feedback via RGBLed ---
      if (distance < levelThreshold) {
        if (currentLevel == 3) {
          rgbLed.setColor(255, 0, 0);
        } else {
          rgbLed.setColor(255, 50, 0);
        }
      } else {
        rgbLed.setColor(255, 0, 0);
      }
      
      // --- Button Handling (with Debouncing) ---
      button.update();
      bool buttonPressed = button.isPressed();
      
      // --- Confirmation State Machine ---
      if (state == WAIT_FOR_CORRECT_VALUE) {
        if (distance < levelThreshold && buttonPressed) {
          confirmStartTime = millis();
          state = CONFIRMING;
        }
      }
      else if (state == CONFIRMING) {
        if (distance >= CONFIRMATION_THRESHOLD) {
          state = WAIT_FOR_CORRECT_VALUE;
        }
        else if (millis() - confirmStartTime >= CONFIRMATION_DELAY) {
          currentStep++;
          Serial.print("Step ");
          Serial.print(currentStep);
          Serial.println(" confirmed!");
          if (currentStep < 3) {
            char stepMsg[17];
            sprintf(stepMsg, "STEP %d OF 3 DONE", currentStep);
            lcd.printMessage(stepMsg, "KEEP GOING", 2000);
          }
          else {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Vault opened!");
            lcd.setCursor(0, 1);
            lcd.print("Congrats!");
            delay(2000);
            lcd.clear();
          }
          state = WAIT_FOR_CORRECT_VALUE;
          lastPrintedValue = -100;
        }
      }
    }
    else if (!safeOpened) {
      // Set vault as opened.
      safeOpened = true;
      // Turn off any ongoing tones and give positive LED feedback.
      noTone(PIN_BUZZER); 
      rgbLed.setColor(0, 255, 0);
      Serial.println("Congratulations! Vault opened!");
      buzzer.playSuccessMelody();
      // Update global timer with the remaining time.
      uint32_t timeElapsed = millis() - gameStartTime;
      uint32_t remaining = (gameDuration > timeElapsed) ? (gameDuration - timeElapsed) : 0;
      gameDuration = remaining;
      // Delay briefly for feedback, then exit the loop.
      delay(1000);
      break;
    }
    
    // --- Vault Opened Behavior: LED Blink ---
    if (safeOpened) {
      static unsigned long lastBlink = 0;
      static bool ledState = false;
      if (millis() - lastBlink > 500) {
        ledState = !ledState;
        rgbLed.setColor(0, ledState ? 255 : 0, 0);
        lastBlink = millis();
      }
    }
    
    delay(10);
  }
}
