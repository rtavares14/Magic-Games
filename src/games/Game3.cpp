#include "Game3.h"
#include <Arduino.h>
#include "LCD.h"
#include "KeyLed.h"
#include "RGBLed.h"
#include "Buzzer.h"
#include "Button.h"
#include "pins.h"

extern LCD lcd;
extern KeyLed keyLed;
extern RGBLed rgb;
extern Buzzer buzzer;
extern Button button;

bool updateGame3() {
  enum Game3State {
    GAME3_INIT,
    GAME3_SHOW_COLOR,
    GAME3_USER_GUESS,
    GAME3_VALIDATE,
    GAME3_SUCCESS,
    GAME3_FAIL
  };
  static Game3State gameState = GAME3_INIT;
  static unsigned long stateStart = millis();
  
  // Target color values (multiples of 10)
  static int targetRed = 0, targetGreen = 0, targetBlue = 0;
  // User guess values
  static int guessRed = 0, guessGreen = 0, guessBlue = 0;
  // Current active channel: 0=red, 1=green, 2=blue
  static int currentChannel = 0;
  
  switch(gameState) {
    case GAME3_INIT: {
      // Display game instructions for 3 seconds.
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Color Memory Game");
      lcd.setCursor(0, 1);
      lcd.print("Get ready...");
      if (millis() - stateStart > 3000) {
        // Generate a random target color (multiples of 10)
        targetRed = (random(0, 26)) * 10;   // 0 to 250
        targetGreen = (random(0, 26)) * 10;
        targetBlue = (random(0, 26)) * 10;
        // Initialize user's guess to zero.
        guessRed = 0;
        guessGreen = 0;
        guessBlue = 0;
        currentChannel = 0;
        stateStart = millis();
        char dbg[50];
      sprintf(dbg, "Color to mach : R:%03d G:%03d B:%03d", targetRed, targetGreen, targetBlue);
      Serial.println(dbg);
        gameState = GAME3_SHOW_COLOR;
      }
      break;
    }
    case GAME3_SHOW_COLOR: {
      // Show the target color on the RGB LED.
      rgb.setColor(targetRed, targetGreen, targetBlue);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Memorize this color!");
      lcd.setCursor(0, 1);
      lcd.print("Press btn when ready");
      // Allow the user to press the button to hide the color early.
      if (button.isPressed() || (millis() - stateStart > 3000)) {
        rgb.setColor(0, 0, 0);  // Hide the target color.
        stateStart = millis();
        gameState = GAME3_USER_GUESS;
        delay(200);  // Debounce delay.
      }
      break;
    }
    case GAME3_USER_GUESS: {
      // Allow the user to recreate the color using keys and the potentiometer.
      uint8_t keys = keyLed.readButtons();
      if (keys & 0x80) {  // Reset guess.
        guessRed = 0;
        guessGreen = 0;
        guessBlue = 0;
        currentChannel = 0;
        delay(200);
      }
      else if (keys & 0x01) {  // Key 1 selects Red.
        currentChannel = 0;
        delay(200);
      }
      else if (keys & 0x02) {  // Key 2 selects Green.
        currentChannel = 1;
        delay(200);
      }
      else if (keys & 0x04) {  // Key 3 selects Blue.
        currentChannel = 2;
        delay(200);
      }
      
      // Read the potentiometer and map its value to discrete steps.
      int potValue = analogRead(PIN_POT);
      int step = map(potValue, 0, 1023, 0, 53);
      int discreteValue = step * 5;
      
      // Update the guess for the current active channel.
      if (currentChannel == 0) {
        guessRed = discreteValue;
      } else if (currentChannel == 1) {
        guessGreen = discreteValue;
      } else if (currentChannel == 2) {
        guessBlue = discreteValue;
      }
      
      // Show the user's current guess on the RGB LED.
      rgb.setColor(guessRed, guessGreen, guessBlue);
      
      // Display the guess on the LCD.
      char line0[17];
      char line1[17];
      sprintf(line0, "R:%03d G:%03d", guessRed, guessGreen);
      sprintf(line1, "B:%03d  btn:Done", guessBlue);
      for (int i = strlen(line0); i < 16; i++) line0[i] = ' ';
      line0[16] = '\0';
      for (int i = strlen(line1); i < 16; i++) line1[i] = ' ';
      line1[16] = '\0';
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(line0);
      lcd.setCursor(0, 1);
      lcd.print(line1);
      
      // When the user presses the button, move to validation.
      if (button.isPressed()) {
        stateStart = millis();
        gameState = GAME3_VALIDATE;
        delay(200);
      }
      break;
    }
    case GAME3_VALIDATE: {
      // Print debug information.
      char dbg[50];
      sprintf(dbg, "Color to mach : R:%03d G:%03d B:%03d", targetRed, targetGreen, targetBlue);
      Serial.println(dbg);
      sprintf(dbg, "Color sent:  R:%03d G:%03d B:%03d", guessRed, guessGreen, guessBlue);
      Serial.println(dbg);
      
      // Check if the user's guess is within a tolerance of 15 for each channel.
      bool redOk = abs(guessRed - targetRed) <= 15;
      bool greenOk = abs(guessGreen - targetGreen) <= 15;
      bool blueOk = abs(guessBlue - targetBlue) <= 15;
      if (redOk && greenOk && blueOk) {
        Serial.println("Right color congrats");
        gameState = GAME3_SUCCESS;
        stateStart = millis();
        buzzer.playSuccessMelody();
      } else {
        Serial.println("Wrong Color");
        gameState = GAME3_FAIL;
        stateStart = millis();
        buzzer.playErrorTone();
      }
      break;
    }
    case GAME3_SUCCESS: {
      // Display a success message.
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Correct! Well");
      lcd.setCursor(0, 1);
      lcd.print("done!");
      rgb.setColor(guessRed, guessGreen, guessBlue);
      if (millis() - stateStart > 2000) {
        return true;  // End the game.
      }
      break;
    }
    case GAME3_FAIL: {
      // Display a failure message.
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Wrong color!");
      lcd.setCursor(0, 1);
      lcd.print("Try again...");
      if (millis() - stateStart > 2000) {
        // Generate a new target color and restart the guessing phase.
        targetRed = (random(0, 26)) * 10;
        targetGreen = (random(0, 26)) * 10;
        targetBlue = (random(0, 26)) * 10;
        guessRed = 0;
        guessGreen = 0;
        guessBlue = 0;
        currentChannel = 0;
        stateStart = millis();
        gameState = GAME3_SHOW_COLOR;
      }
      break;
    }
  }
  return false;  // Game is still in progress.
}
