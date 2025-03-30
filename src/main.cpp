#include <Arduino.h>
#include "pins.h"
#include "Game1.h"
#include "Game2.h"
#include "Game3.h"
#include "RGBLed.h"
#include "LCD.h"
#include "KeyLed.h"
#include "Buzzer.h"
#include "Button.h"

// Total time: 10 minutes
extern const uint32_t TOTAL_TIME = 600000UL;
uint32_t globalStartTime;

// Global shared objects.
LCD lcd(0x27, 16, 2);
KeyLed keyLed(STB_PIN, CLK_PIN, DIO_PIN);
RGBLed rgb;
Buzzer buzzer(PIN_BUZZER);
Button button(PIN_BUTTON, 50);

// New global variables for button press count.
int totalButtonPresses = 0;
int currentGamePresses = 0;

// Application States (note: STATE_GAME_OVER removed; TIME_UP is now game over)
enum AppState
{
  STATE_INTRO,
  STATE_GAME1,
  STATE_LOADING1,
  STATE_GAME2,
  STATE_LOADING2,
  STATE_GAME3,
  STATE_LOADING3,
  STATE_GAME4,   // For future games, if any.
  STATE_TIME_UP, // Time up becomes game over.
  STATE_GAME_WON // All games finished with time remaining.
};

AppState currentState = STATE_INTRO;
uint32_t stateStartTime = 0;

// Update the timer display using the current game button press count.
void updateTimerDisplay()
{
  uint32_t elapsed = millis() - globalStartTime;
  keyLed.displayTime(elapsed, TOTAL_TIME, currentGamePresses);
}

// Intro State (same as before)
void updateIntro()
{
  uint32_t now = millis();
  uint32_t elapsedState = now - stateStartTime;
  static int lastMessageIndex = -1;
  int messageIndex = -1;

  if (elapsedState < 2000)
  {
    messageIndex = 0;
  }
  else if (elapsedState < 4000)
  {
    messageIndex = 1;
  }
  else if (elapsedState < 6000)
  {
    messageIndex = 2;
  }
  else
  {
    currentGamePresses = 0;
    currentState = STATE_GAME1;
    stateStartTime = now;
    lastMessageIndex = -1;
    return;
  }

  if (messageIndex != lastMessageIndex)
  {
    lcd.clear();
    switch (messageIndex)
    {
    case 0:
      lcd.lcdShow("New Adventure", "has begun!");
      break;
    case 1:
      lcd.lcdShow("Have Fun", "Good Luck!");
      Serial.println("------------------------------------");
      Serial.println("-------------Main-Loop--------------");
      Serial.println("The games will begin soon.");
      Serial.println("Timer: 10 minutes");
      break;
    case 2:
      lcd.lcdShow("Starting Games", "with 10m timer...");
      break;
    }
    lastMessageIndex = messageIndex;
  }
}

// Reusable loading function (unchanged except duration remains 3000ms)
void updateLoadingGame(const char *loadingMessage, AppState nextState)
{
  uint32_t now = millis();
  uint32_t elapsedState = now - stateStartTime;
  static int lastMsgIndex = -1;
  int messageIndex = -1;

  if (elapsedState < 3000)
  {
    messageIndex = 0;
  }
  else
  {
    currentGamePresses = 0;
    currentState = nextState;
    stateStartTime = now;
    lastMsgIndex = -1;
    return;
  }

  if (messageIndex != lastMsgIndex)
  {
    lcd.clear();
    lcd.lcdShow(loadingMessage, "Loading...");
    lastMsgIndex = messageIndex;
  }

  rgb.loadingAnimation(elapsedState);
}

// Game state functions
void game1()
{
  bool finished = updateGame1();
  if (finished)
  {
    currentState = STATE_LOADING1;
    stateStartTime = millis();
  }
}

void game2()
{
  bool finished = updateGame2();
  if (finished)
  {
    currentState = STATE_LOADING2;
    stateStartTime = millis();
  }
}

void game3()
{
  bool finished = updateGame3();
  if (finished)
  {
    currentState = STATE_GAME_WON;
    stateStartTime = millis();
  }
}

void updateTimeUp()
{
  uint32_t now = millis();
  uint32_t elapsedState = now - stateStartTime;
  static int lastMsgIndex = -1;
  int msgIndex = -1;
  rgb.setColor(255, 0, 0);

  if (elapsedState < 3000)
  {
      msgIndex = 0;
      if (msgIndex != lastMsgIndex)
      {
          lcd.lcdShow("You are forever", "Lost ....");
          lastMsgIndex = msgIndex;
      }
  }
  else
  {
      msgIndex = 1;
      if (msgIndex != lastMsgIndex)
      {
          lcd.lcdShow("Reset? Press btn", "");
          lastMsgIndex = msgIndex;
      }
  }
}

// New function: Update Game Won state with celebration.
void updateGameWon()
{
  uint32_t now = millis();
  uint32_t elapsedState = now - stateStartTime;
  static int lastMsgIndex = -1;
  int msgIndex = -1;

  if (elapsedState < 3000)
  {
    msgIndex = 0;
    if (msgIndex != lastMsgIndex)
    {
      lcd.clear();
      lcd.lcdShow("GAME WON!", "Congratulations!");
      lastMsgIndex = msgIndex;
    }
  }
  else
  {
    // Blink green LED every 500ms and play a celebratory tone.
    if ((elapsedState / 500) % 2 == 0)
      rgb.setColor(0, 255, 0);
    else
      rgb.setColor(0, 0, 0);

    if (elapsedState % 3000 < 50) // Play success melody periodically.
    {
      buzzer.playSuccessMelody();
    }
  }
}

// Setup & Main Loop
void setup()
{
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

void loop()
{
  updateTimerDisplay();
  button.update(); // Called once per loop

  // Only count button presses when in game states.
  if (currentState == STATE_GAME1 || currentState == STATE_GAME2 || currentState == STATE_GAME3)
  {
    static bool lastButtonState = false;
    bool currentButtonState = button.isPressed();
    if (currentButtonState && !lastButtonState)
    {
      totalButtonPresses++;
      currentGamePresses++;
    }
    lastButtonState = currentButtonState;
  }

  // Check global time remaining. If less than or equal to 1 second, transition to TIME_UP state.
  uint32_t elapsedGlobal = millis() - globalStartTime;
  if (TOTAL_TIME - elapsedGlobal <= 1000 && currentState != STATE_TIME_UP && currentState != STATE_GAME_WON)
  {
    currentState = STATE_TIME_UP;
    stateStartTime = millis();
  }

  switch (currentState)
  {
  case STATE_INTRO:
    updateIntro();
    break;
  case STATE_GAME1:
    game1();
    break;
  case STATE_LOADING1:
    updateLoadingGame("Game 2 Loading", STATE_GAME2);
    break;
  case STATE_GAME2:
    game2();
    break;
  case STATE_LOADING2:
    updateLoadingGame("Game 3 Loading", STATE_GAME3);
    break;
  case STATE_GAME3:
    game3();
    break;
  case STATE_LOADING3:
    updateLoadingGame("Game 4 Loading", STATE_GAME4);
    break;
  case STATE_TIME_UP:
    updateTimeUp();
    break;
  case STATE_GAME_WON:
    updateGameWon();
    break;
  }
  delay(10);
}
