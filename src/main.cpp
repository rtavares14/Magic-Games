#include <Arduino.h>
#include "pins.h"
#include "games/Game1.h"   // These now provide non-blocking update functions.
#include "games/Game2.h"
#include "components/RGBLed.h"
#include "components/LCD.h"
#include "components/KeyLed.h"
#include "components/Buzzer.h"
#include "components/Button.h"

// Total time: 10 minutes (600,000 ms)
extern const uint32_t TOTAL_TIME = 600000UL;

// Global timer (set once)
uint32_t globalStartTime;

// Global shared objects.
LCD lcd(0x27, 16, 2);
KeyLed keyLed(STB_PIN, CLK_PIN, DIO_PIN);
RGBLed rgb;
Buzzer buzzer(PIN_BUZZER);
Button button(PIN_BUTTON, 50);

//---------------------------------------------------------------------
// Application States
//---------------------------------------------------------------------
enum AppState {
  STATE_INTRO,
  STATE_GAME1,
  STATE_LOADING,
  STATE_GAME2,
  STATE_GAME_OVER
};

AppState currentState = STATE_INTRO;
uint32_t stateStartTime = 0;

// Shared game variable (e.g. for attempt count)
int currentGameAttempt = 0;

//---------------------------------------------------------------------
// Helper: Update the timer display (runs every loop)
//---------------------------------------------------------------------
void updateTimerDisplay() {
  uint32_t elapsed = millis() - globalStartTime;
  keyLed.displayTime(elapsed, TOTAL_TIME, currentGameAttempt);
}

//---------------------------------------------------------------------
// Non-blocking Intro State using millis() (no delay)
//---------------------------------------------------------------------
void updateIntro() {
  uint32_t now = millis();
  uint32_t elapsedState = now - stateStartTime;
  // We'll use a static variable to update the display only when needed.
  static int lastMessageIndex = -1;
  int messageIndex = -1;
  
  if (elapsedState < 2000) {
    messageIndex = 0;
  }
  else if (elapsedState < 4000) {
    messageIndex = 1;
  }
  else if (elapsedState < 6000) {
    messageIndex = 2;
  }
  else if (elapsedState < 8000) {
    messageIndex = 3;
  }
  else {
    // Transition to Game 1 state
    currentState = STATE_GAME1;
    stateStartTime = now;
    lastMessageIndex = -1;
    return;
  }
  
  // Only update the display if the message state has changed.
  if (messageIndex != lastMessageIndex) {
    lcd.clear();
    switch(messageIndex) {
      case 0:
        lcd.setCursor(0, 0);
        lcd.print("New Adventure");
        lcd.setCursor(0, 1);
        lcd.print("has begun!");
        break;
      case 1:
        lcd.setCursor(0, 0);
        lcd.print("Have Fun");
        lcd.setCursor(0, 1);
        lcd.print("Good Luck");
        break;
      case 2:
        lcd.setCursor(0, 0);
        lcd.print("Starting Games");
        lcd.setCursor(0, 1);
        lcd.print("with 10m timer...");
        break;
    }
    lastMessageIndex = messageIndex;
  }
}

//---------------------------------------------------------------------
// Non-blocking Game1 State
//---------------------------------------------------------------------
void updateGame1() {
  // updateGame1NonBlocking() must be refactored to use non-blocking millis()-based logic.
  bool finished = updateGame1NonBlocking();
  if (finished) {
    currentState = STATE_LOADING;
    stateStartTime = millis();
  }
}

//---------------------------------------------------------------------
// Non-blocking Loading State (between games)
//---------------------------------------------------------------------
void updateLoading() {
  uint32_t now = millis();
  uint32_t elapsedState = now - stateStartTime;
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Game 2 Loading");

  // Cycle through colors non-blocking.
  rgb.loadingAnimation(elapsedState);
  
  // After 5 seconds, transition to Game 2.
  if (elapsedState >= 5000) {
    currentState = STATE_GAME2;
    stateStartTime = now;
  }
}

//---------------------------------------------------------------------
// Non-blocking Game2 State
//---------------------------------------------------------------------
void updateGame2() {
  bool finished = updateGame2NonBlocking();
  if (finished) {
    currentState = STATE_GAME_OVER;
    stateStartTime = millis();
  }
}

//---------------------------------------------------------------------
// Game Over State
//---------------------------------------------------------------------
void updateGameOver() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Game Over!");
}

//---------------------------------------------------------------------
// Setup & Main Loop
//---------------------------------------------------------------------
void setup() {
  Serial.begin(9600);
  lcd.begin();
  keyLed.begin();
  rgb.begin();
  buzzer.begin();
  button.begin();

  globalStartTime = millis();
  currentState = STATE_INTRO;
  stateStartTime = millis();
}

void loop() {
  // Always update the timer display.
  updateTimerDisplay();

  // Update global button state.
  button.update();
  
  // Dispatch state updates.
  switch (currentState) {
    case STATE_INTRO:
      updateIntro();
      break;
    case STATE_GAME1:
      updateGame1();
      break;
    case STATE_LOADING:
      updateLoading();
      break;
    case STATE_GAME2:
      updateGame2();
      break;
    case STATE_GAME_OVER:
      updateGameOver();
      break;
  }

  delay(10);  // Short delay to prevent the loop from running too fast.
}
