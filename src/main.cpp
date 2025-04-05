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
#include "Globals.h"

// Total time: 10 minutes
extern const uint32_t TOTAL_TIME = 600000UL;
uint32_t globalStartTime;

// Global shared objects.
LCD lcd(0x27, 16, 2);
KeyLed keyLed(STB_PIN, CLK_PIN, DIO_PIN);
RGBLed rgb;
Buzzer buzzer(PIN_BUZZER);
Button button(PIN_BUTTON, 50);

// Global variables for button press count.
int totalButtonPresses = 0;
int currentGamePresses = 0;

int game1FinalScore = 0;
int game2FinalScore = 0;
int game3FinalScore = 0;
int game4FinalScore = 0;
int totalScore = 0;

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
  STATE_TIME_UP,
  STATE_GAME_WON
};

AppState currentState = STATE_INTRO;
uint32_t stateStartTime = 0;

// Update the timer display using the current game button press count.
void updateTimerDisplay()
{
  if (currentState == STATE_GAME_WON)
    return;
  uint32_t elapsed = millis() - globalStartTime;
  keyLed.displayTime(elapsed, TOTAL_TIME, currentGamePresses);
}

// Intro State
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

// loading function 
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
  bool finished = updateGame3();
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
  buzzer.playGameOverMelody();

  if (elapsedState < 3000)
  {
    msgIndex = 0;
    if (msgIndex != lastMsgIndex)
    {
      lcd.lcdShow("You are forever", "Lost ....");
      lastMsgIndex = msgIndex;
    }
  }
}

// Game Won state with celebration.
void updateGameWon()
{
  static bool printedGameWon = false;
  totalScore = game1FinalScore + game2FinalScore + game3FinalScore + game4FinalScore;
  uint32_t now = millis();
  uint32_t elapsedState = now - stateStartTime;
  static int lastMsgIndex = -1;
  int msgIndex = -1;

  if (!printedGameWon)
  {
    Serial.println("------------------------------------");
    Serial.println("--------------Game-Won--------------");
    Serial.println("Game Won!");
    Serial.print("BTN PRESSES: ");
    Serial.println(totalButtonPresses);
    Serial.print("POINTS: ");
    Serial.println(totalScore);
    printedGameWon = true;
  }

  uint32_t cycleTime = elapsedState % 6000;

  if ((elapsedState / 500) % 2 == 0)
    rgb.setColor(0, 255, 0);
  else
    rgb.setColor(0, 0, 0);

  if (elapsedState % 5000 < 50)
    buzzer.playSuccessMelody();

  // For the first half of the cycle, show "GAME WON!" message.
  if (cycleTime < 5000)
  {
    lcd.clear();
    lcd.lcdShow("GAME WON!", "Congratulations!");
  }
  else
  {
    String line0 = "BTN PRESSES: " + String(totalButtonPresses);
    String line1 = "POINTS: " + String(totalScore);
    lcd.clear();
    lcd.lcdShow(line0.c_str(), line1.c_str());
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
  button.update();

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
