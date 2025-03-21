#include "Game2.h"
#include <Arduino.h>
#include "../components/LCD.h"
#include "../components/KeyLed.h"
#include "../components/Buzzer.h"
#include "../components/RGBLed.h"
#include "../components/Button.h"
#include "pins.h"

// Use the global game timer from main.cpp (this now holds the remaining time from Game1)
extern uint32_t gameDuration;

// --- Game Constants ---
const int MELODY_LENGTH = 8;
const int KEY_DEBOUNCE_DELAY = 150;      // ms for key debouncing
const unsigned long BUTTON_DEBOUNCE_DELAY = 50; // ms for confirmation button

// --- Game Variables ---
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

unsigned long gameStartTime;
int attemptCount = 0;
String userInput = "";
unsigned long lastKeyPressTime = 0;
uint8_t lastButtons = 0;
char keyDigits[8] = {'1','2','3','4','5','6','7','8'};
int noteFrequencies[8] = {261, 293, 329, 349, 392, 440, 493, 523};

// --- Helper Functions ---

// Display the tip for the next expected note on the LCD.
void showTip(LCD &lcd, int currentIndex) {
  lcd.clear();
  if (currentIndex < MELODY_LENGTH) {
    char tipHeader[17];
    sprintf(tipHeader, "Tip for note %d", currentIndex + 1);
    lcd.setCursor(0, 0);
    lcd.print(tipHeader);
    lcd.setCursor(0, 1);
    // Ensure tip fits in 16 characters.
    String tip = tips[currentIndex];
    if (tip.length() > 16) {
      tip = tip.substring(0, 16);
    }
    lcd.print(tip.c_str());
  }
}

// Play a success melody using the buzzer.
void playSuccessMelody(Buzzer &buzzer) {
  int melody[] = {261, 293, 329, 349, 392, 440, 493, 523};
  buzzer.playTone(melody[0], 400);
  delay(500);
  for (int i = 1; i < 7; i++) {
    buzzer.playTone(melody[i], 200);
    delay(250);
  }
  buzzer.playTone(melody[7], 400);
  delay(500);
}

// Play an error tone using the buzzer.
void playErrorTone(Buzzer &buzzer) {
  buzzer.playTone(300, 300);
}

// --- Main Game2 Function ---
void runGame2() {
  // Instantiate components.
  LCD lcd(0x27, 16, 2);
  KeyLed keyLed(STB_PIN, CLK_PIN, DIO_PIN);
  Buzzer buzzer(PIN_BUZZER);
  RGBLed rgbLed;     // Uses PIN_RED, PIN_GREEN, PIN_BLUE from pins.h
  Button button(PIN_BUTTON, BUTTON_DEBOUNCE_DELAY);
  
  // Initialize components.
  lcd.begin();
  keyLed.begin();
  buzzer.begin();
  rgbLed.begin();
  button.begin();
  
  // Use the RGB LED's loading effect before starting game logic (2 seconds).
  rgbLed.loadingEffect(2000);
  
  // Show initial info.
  lcd.printMessage("Mystery Melody", "Find the tune!", 3000);
  showTip(lcd, userInput.length());
  
  // Set initial LED color (blue).
  rgbLed.setColor(0, 0, 255);
  
  // Record game start time.
  gameStartTime = millis();
  
  // Main game loop.
  while (true) {
    unsigned long elapsed = millis() - gameStartTime;
    
    // --- Timer Check ---
    if (elapsed >= gameDuration) {
      // Time's up: update display and show game over.
      keyLed.displayTime(gameDuration, gameDuration, attemptCount);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Time's up!");
      lcd.setCursor(0, 1);
      lcd.print("Game Over!");
      buzzer.playTone(400, 200);
      delay(1000);
      noTone(PIN_BUZZER);
      while(true); // Halt here.
    }
    
    // Update KeyLed display with timer and attempt count.
    keyLed.displayTime(elapsed, gameDuration, attemptCount);
    
    // --- Process TM1638 Keys for Melody Input ---
    uint8_t keys = keyLed.readButtons();
    int pressedCount = 0;
    int pressedIndex = -1;
    
    for (int i = 0; i < 8; i++) {
      bool pressed = (keys & (1 << i)) != 0;
      if (pressed) {
        pressedCount++;
        if (pressedIndex == -1) {
          pressedIndex = i;
        }
        // Light up the key LED.
        keyLed.setLED(i, true);
      } else {
        keyLed.setLED(i, false);
      }
    }
    
    // If exactly one key is pressed, record it (with debouncing).
    if (pressedCount == 1 && (millis() - lastKeyPressTime > KEY_DEBOUNCE_DELAY) &&
        ((lastButtons & (1 << pressedIndex)) == 0)) {
      userInput += keyDigits[pressedIndex];
      buzzer.playTone(noteFrequencies[pressedIndex], 200);
      delay(200);
      noTone(PIN_BUZZER);
      lastKeyPressTime = millis();
      showTip(lcd, userInput.length());
    }
    lastButtons = keys;
    
    // --- Process Confirmation Button ---
    button.update();
    if (button.isPressed() && userInput.length() > 0) {
      // Compare only the first MELODY_LENGTH digits.
      String compareInput = userInput.substring(0, min((int)userInput.length(), MELODY_LENGTH));
      if (compareInput.equals(targetMelody)) {
        // Correct melody entered.
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Correct Tune!");
        lcd.setCursor(0, 1);
        lcd.print("Well done!");
        rgbLed.setColor(0, 255, 0);  // Green LED.
        playSuccessMelody(buzzer);
        break;  // End game on success.
      } else {
        // Wrong melody.
        attemptCount++;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Wrong Tune!");
        lcd.setCursor(0, 1);
        lcd.print("Try again!");
        rgbLed.setColor(255, 0, 0);  // Red LED.
        playErrorTone(buzzer);
        delay(1000);
        rgbLed.setColor(0, 0, 255);  // Back to blue.
        userInput = "";
        // Subtract 30 seconds from the timer.
        gameStartTime -= 30000;
        showTip(lcd, 0);
      }
    }
    
    delay(10);
  }
}
