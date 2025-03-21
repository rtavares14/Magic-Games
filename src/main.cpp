#include <Arduino.h>
#include "pins.h"
#include "games/Game1.h"
#include "games/Game2.h"
#include "components/RGBLed.h"
#include "components/LCD.h"

// Global timer duration for all games: 10 minutes (600,000 ms)
uint32_t gameDuration = 600000UL;

void setup() {
  Serial.begin(9600);
  Serial.println("Starting new adventure...");
  delay(1000);

  // Show intro messages on LCD (each message is displayed for 2 seconds).
  LCD lcd(0x27, 16, 2);
  lcd.begin();
  lcd.printMessage("New Adventure", "Begins Now", 2000);
  lcd.printMessage("Have Fun", "", 2000);
  lcd.printMessage("Good Luck", "", 2000);

  Serial.println("Starting Game 1 with 10-minute timer...");
  delay(1000);

  // Run Game 1. When it returns, gameDuration holds the remaining time.
  runGame1();

  Serial.print("Game 1 finished. Remaining time: ");
  Serial.println(gameDuration);
  Serial.println("Loading Game 2...");

  // Use the LCD to show the loading message and then perform the RGB loading effect.
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Game 2 Loading");

  RGBLed rgb;
  rgb.begin();
  rgb.loadingEffect(5000);  // 5-second loading effect

  // Now start Game 2 (which will use the updated global gameDuration)
  runGame2();
}

void loop() {
  // Games run as blocking functions.
  delay(100);
}
