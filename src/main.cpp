#include <Arduino.h>
#include "pins.h"
#include "Game1.h"
#include "Game2.h"
#include "Game3.h"
#include "Game4.h"
#include "RGBLed.h"
#include "LCD.h"
#include "KeyLed.h"
#include "Buzzer.h"
#include "Button.h"
#include "Globals.h"

// Global configuration constants
static const uint8_t LCD_I2C_ADDRESS = 0x27;
static const int LCD_COLUMNS = 16;
static const int LCD_ROWS = 2;
static const unsigned long BUTTON_DEBOUNCE_MS = 50;
static const unsigned long INTRO_MESSAGE_INTERVAL_MS = 2000;
static const unsigned long LOADING_SCREEN_DURATION_MS = 3000;
static const unsigned long TIME_UP_WARNING_MS = 1000;
static const unsigned long TIME_UP_DISPLAY_DURATION_MS = 3000;
static const unsigned long CELEBRATION_CYCLE_MS = 6000;
static const unsigned long CELEBRATION_BLINK_INTERVAL_MS = 500;
static const unsigned long CELEBRATION_MELODY_INTERVAL_MS = 5000;
static const unsigned long CELEBRATION_MELODY_WINDOW_MS = 50;
static const int LED_RED_R = 255;
static const int LED_RED_G = 0;
static const int LED_RED_B = 0;

// Total time for all games: 10 minutes
extern const uint32_t TOTAL_TIME = 600000UL;
uint32_t globalStartTime;

// Global shared objects
LCD lcd(LCD_I2C_ADDRESS, LCD_COLUMNS, LCD_ROWS);
KeyLed keyLed(STB_PIN, CLK_PIN, DIO_PIN);
RGBLed rgb;
Buzzer buzzer(PIN_BUZZER);
Button button(PIN_BUTTON, BUTTON_DEBOUNCE_MS);

// Global variables for button press counts
int totalButtonPresses = 0;
int currentGamePresses = 0;

// Scores for each game and total score
int game1FinalScore = 0;
int game2FinalScore = 0;
int game3FinalScore = 0;
int game4FinalScore = 0;
int totalScore = 0;

// Application state definitions
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

// Update the 7-seg display with elapsed time and button presses
void updateTimerDisplay()
{
  if (currentState == STATE_GAME_WON)
  {
    return;
  }
  uint32_t elapsed = millis() - globalStartTime;
  keyLed.displayTime(elapsed, TOTAL_TIME, currentGamePresses);
}

// Intro state: show introductory messages then transition to Game1
void updateIntro()
{
  uint32_t now = millis();
  uint32_t elapsedState = now - stateStartTime;
  static int lastMessageIndex = -1;
  int messageIndex = -1;

  if (elapsedState < INTRO_MESSAGE_INTERVAL_MS)
  {
    messageIndex = 0;
  }
  else if (elapsedState < 2 * INTRO_MESSAGE_INTERVAL_MS)
  {
    messageIndex = 1;
  }
  else if (elapsedState < 3 * INTRO_MESSAGE_INTERVAL_MS)
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

// Loading state: brief loading screen between games
void updateLoadingGame(const char *loadingMessage, AppState nextState)
{
  uint32_t now = millis();
  uint32_t elapsedState = now - stateStartTime;
  static int lastMsgIndex = -1;
  int messageIndex = -1;

  if (elapsedState < LOADING_SCREEN_DURATION_MS)
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

// Game state handlers calling each game's update function
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
    currentState = STATE_LOADING3;
    stateStartTime = millis();
  }
}

void game4()
{
  bool finished = updateGame4();
  if (finished)
  {
    uint32_t elapsedGlobal = millis() - globalStartTime;
    if (elapsedGlobal >= TOTAL_TIME)
    {
      currentState = STATE_TIME_UP;
    }
    else
    {
      currentState = STATE_GAME_WON;
    }
    stateStartTime = millis();
  }
}

// Handle global timeout (no games completed in time)
void updateTimeUp()
{
  uint32_t now = millis();
  uint32_t elapsedState = now - stateStartTime;
  static int lastMsgIndex = -1;
  int msgIndex = -1;
  rgb.setColor(LED_RED_R, LED_RED_G, LED_RED_B);
  buzzer.playGameOverMelody();

  if (elapsedState < TIME_UP_DISPLAY_DURATION_MS)
  {
    msgIndex = 0;
    if (msgIndex != lastMsgIndex)
    {
      lcd.lcdShow("You are forever", "Lost...");
      lastMsgIndex = msgIndex;
    }
  }
}

// Game Won state: celebration display and stats
void updateGameWon()
{
  static bool printedGameWon = false;
  // On first entry, clear the LCD and print serial info.
  // Calculate total score.
  totalScore = game1FinalScore + game2FinalScore + game3FinalScore + game4FinalScore;
  
  if (!printedGameWon)
  {
    lcd.clear();
    printedGameWon = true;
    Serial.println("------------------------------------");
    Serial.println("--------------Game-Won--------------");
    Serial.println("Game Won!");
    Serial.print("BTN PRESSES: ");
    Serial.println(totalButtonPresses);
    Serial.print("POINTS: ");
    Serial.println(totalScore);
  }

  uint32_t now = millis();
  uint32_t elapsedState = now - stateStartTime;
  uint32_t cycleTime = elapsedState % CELEBRATION_CYCLE_MS;

  // Blink RGB LED green on and off.
  if ((elapsedState / CELEBRATION_BLINK_INTERVAL_MS) % 2 == 0)
  {
    rgb.setColor(0, 255, 0);
  }
  else
  {
    rgb.setColor(0, 0, 0);
  }

  // Play success melody at the start of each interval.
  if (elapsedState % CELEBRATION_MELODY_INTERVAL_MS < CELEBRATION_MELODY_WINDOW_MS)
  {
    buzzer.playSuccessMelody();
  }

  // Alternate between congratulatory message and final stats.
  if (cycleTime < 5000)
  {
    lcd.lcdShow("GAME WON!", "Congratulations!");
  }
  else
  {
    char line0[17];
    char line1[17];
    snprintf(line0, 17, "BTN:%d", totalButtonPresses);
    snprintf(line1, 17, "PTS:%d", totalScore);
    lcd.lcdShow(line0, line1);
  }
}

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

  // Count button presses during games
  if (currentState == STATE_GAME1 || currentState == STATE_GAME2 ||
      currentState == STATE_GAME3 || currentState == STATE_GAME4)
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

  // Check if time is nearly up and trigger time-up state if needed
  uint32_t elapsedGlobal = millis() - globalStartTime;
  if (TOTAL_TIME - elapsedGlobal <= TIME_UP_WARNING_MS &&
      currentState != STATE_TIME_UP && currentState != STATE_GAME_WON)
  {
    currentState = STATE_TIME_UP;
    stateStartTime = millis();
  }

  // State machine: call update function for the current state
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
  case STATE_GAME4:
    game4();
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
