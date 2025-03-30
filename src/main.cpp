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

// Application States
enum AppState
{
  STATE_INTRO,
  STATE_GAME1,
  STATE_LOADING1,
  STATE_GAME2,
  STATE_LOADING2,
  STATE_GAME3,
  STATE_LOADING3,
  STATE_GAME4,
  STATE_GAME_OVER,
  STATE_GAME_WON
};

AppState currentState = STATE_INTRO;
uint32_t stateStartTime = 0;

// Update the timer display using the current game button press count.
void updateTimerDisplay()
{
  uint32_t elapsed = millis() - globalStartTime;
  keyLed.displayTime(elapsed, TOTAL_TIME, currentGamePresses);
}

// Intro State using millis()
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
    // Reset per-game press counter before starting Game 1.
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

// Game1 State
void game1()
{
  bool finished = updateGame1();
  if (finished)
  {
    currentState = STATE_LOADING1;
    stateStartTime = millis();
  }
}

// Loading State (between games)
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
        // After 3000ms, reset the per-game press counter, set the next state, and update stateStartTime.
        currentGamePresses = 0;
        currentState = nextState;
        stateStartTime = now;
        lastMsgIndex = -1;
        return;
    }
    
    // Only update the LCD if the message index has changed.
    if (messageIndex != lastMsgIndex)
    {
        lcd.clear();
        lcd.lcdShow(loadingMessage, "Loading...");
        lastMsgIndex = messageIndex;
    }
    
    // Run the loading animation using the elapsed time.
    rgb.loadingAnimation(elapsedState);
}

// Game2 State
void game2()
{
  bool finished = updateGame2();
  if (finished)
  {
    // Transition to Game3 instead of Game Over.
    currentState = STATE_LOADING2;
    stateStartTime = millis();
  }
}

// Game3 State
void game3()
{
  bool finished = updateGame3();
  if (finished)
  {
    currentState = STATE_GAME_OVER;
    stateStartTime = millis();
  }
}

// Game Over State
void updateGameOver()
{
  static bool printed = false;
  lcd.clear();
  if (!printed)
  {
    lcd.lcdShow("Game Over", "Press button");
    Serial.println("------------------------------------");
    Serial.println("-------------Game-Over!-------------");
    Serial.println("------------------------------------");
    printed = true;

    // If the button is pressed, reset the full game.
    if (button.isPressed())
    {
      printed = false;
      globalStartTime = millis();
      stateStartTime = millis();
      currentGamePresses = 0;
      currentState = STATE_INTRO;
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
  button.update(); // Called only once per loop

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
  case STATE_GAME_OVER:
    updateGameOver();
    break;
  }
  delay(10);
}
