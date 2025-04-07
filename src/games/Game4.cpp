#include "Game4.h"
#include <Arduino.h>
#include "LCD.h"
#include "KeyLed.h"
#include "Buzzer.h"
#include "RGBLed.h"
#include "Button.h"
#include "Potentiometer.h"
#include "pins.h"
#include "Score.h"
#include "Globals.h"
#include <string.h>

// Structure for a trivia question with 5 options.
struct TriviaQuestion
{
  const char *question;
  const char *options[5]; // Options labeled A–E
  int correctIndex;       // 0-based index: 0=A, 1=B, …, 4=E
};

// Updated general knowledge questions (each 16 chars or less).
static const TriviaQuestion questions[] = {
    {"France: Capital",
     {"Paris", "Berlin", "Madrid", "Rome", "Lisbon"},
     0},
    {"Largest planet?",
     {"Earth", "Mars", "Jupiter", "Saturn", "Neptune"},
     2},
    {"Hamlet Author?",
     {"Dickens", "Shakespeare", "Twain", "Tolstoy", "Austen"},
     1},
    {"Water Boils at?",
     {"90°C", "100°C", "110°C", "120°C", "80°C"},
     1},
    {"Element 'O' is?",
     {"Gold", "Oxygen", "Silver", "Iron", "Hydrogen"},
     1},
    {"Largest ocean?",
     {"Atlantic", "Indian", "Arctic", "Southern", "Pacific"},
     4},
    {"Portugal is in?",
     {"Africa", "Asia", "S.America", "Europe", "Australia"},
     3}};

static const int NUM_QUESTIONS = sizeof(questions) / sizeof(questions[0]);

// Timing constants (in milliseconds)
const unsigned long GAME4_INIT_DURATION = 2000;
const unsigned long GAME4_FEEDBACK_DURATION = 2000;
const unsigned long TYPEWRITER_DELAY = 50;
const unsigned long GAME4_OPTION_UPDATE_INTERVAL = 500;
const unsigned long WRONG_FEEDBACK_DURATION = 1200;

// Buzzer tone settings.
const int TONE_SUCCESS_FREQ = 1200;
const unsigned long TONE_SUCCESS_DURATION = 100;
const int TONE_ERROR_FREQ = 400;
const unsigned long TONE_ERROR_DURATION = 100;

// Game state definitions.
enum Game4State
{
  GAME4_INIT,
  GAME4_SHOW_QUESTION,
  GAME4_WAIT_FOR_ANSWER,
  GAME4_WRONG,
  GAME4_SUCCESS,
  GAME4_COMPLETE,
  GAME4_TIME_UP
};

// Global variables to track correct answers.
static int correctCount = 0;
static bool firstTry = true;

bool updateGame4()
{
  // Print welcome message only once.
  static bool welcomePrinted = false;
  if (!welcomePrinted)
  {
    Serial.println("------------------------------------");
    Serial.println("---------------Game-4---------------");
    Serial.println("Answer the questions correctly.");
    welcomePrinted = true;
  }

  static bool finalPrinted = false;
  static Game4State gameState = GAME4_INIT;
  static unsigned long stateStart = millis();
  static int currentQuestion = 0;
  static int selectedOption = 0;
  static uint32_t game4StartTime = 0;
  static int charIndex = 0;
  static unsigned long lastCharTime = 0;
  static char typedQuestion[17] = {0};
  static bool typewriterInit = false;
  static unsigned long lastOptionUpdate = 0;

  // Check global timer expiration.
  uint32_t elapsedGlobal = millis() - globalStartTime;
  if (elapsedGlobal >= TOTAL_TIME)
  {
    return true;
  }

  switch (gameState)
  {
  case GAME4_INIT:
  {
    rgb.setColor(0, 0, 255);
    if (millis() - stateStart < GAME4_INIT_DURATION)
    {
      lcd.updateLCD("Trivia Time!", "Get Ready...");
    }
    else
    {
      currentQuestion = 0;
      selectedOption = 0;
      game4StartTime = millis();
      typewriterInit = false;
      firstTry = true;
      Serial.print("Question ");
      Serial.println(currentQuestion + 1);
      gameState = GAME4_SHOW_QUESTION;
      stateStart = millis();
    }
    break;
  }
  case GAME4_SHOW_QUESTION:
  {
    const char *fullQuestion = questions[currentQuestion].question;
    char truncatedQuestion[17];
    strncpy(truncatedQuestion, fullQuestion, 16);
    truncatedQuestion[16] = '\0';
    int len = strlen(truncatedQuestion);
    if (!typewriterInit)
    {
      charIndex = 0;
      lastCharTime = millis();
      memset(typedQuestion, 0, sizeof(typedQuestion));
      typewriterInit = true;
    }
    if (charIndex < len)
    {
      if (millis() - lastCharTime >= TYPEWRITER_DELAY)
      {
        typedQuestion[charIndex] = truncatedQuestion[charIndex];
        charIndex++;
        lastCharTime = millis();
      }
      char optionLine[17];
      snprintf(optionLine, 17, "Select: A");
      lcd.updateLCD(typedQuestion, optionLine);
    }
    else
    {
      Serial.print("Q");
      Serial.print(currentQuestion + 1);
      Serial.print(": ");
      Serial.println(truncatedQuestion);
      gameState = GAME4_WAIT_FOR_ANSWER;
      stateStart = millis();
      lastOptionUpdate = millis();
    }
    break;
  }
  case GAME4_WAIT_FOR_ANSWER:
  {
    rgb.setColor(0, 0, 255);
    if (millis() - lastOptionUpdate >= GAME4_OPTION_UPDATE_INTERVAL)
    {
      int potValue = analogRead(PIN_POT);
      int mappedOption = map(potValue, 0, 1023, 0, 5);
      if (mappedOption > 4)
        mappedOption = 4;
      selectedOption = mappedOption;
      char questionLine[17];
      strncpy(questionLine, questions[currentQuestion].question, 16);
      questionLine[16] = '\0';
      char optionLine[17];
      snprintf(optionLine, 17, "%c: %s", 'A' + selectedOption, questions[currentQuestion].options[selectedOption]);
      lcd.updateLCD(questionLine, optionLine);
      lastOptionUpdate = millis();
    }
    if (button.isPressed())
    {
      Serial.print("Question ");
      Serial.print(currentQuestion + 1);
      Serial.print(" Answer: ");
      Serial.print((char)('A' + selectedOption));
      if (selectedOption == questions[currentQuestion].correctIndex)
      {
        Serial.println(" - Correct");
        if (firstTry)
        {
          correctCount++;
        }
        gameState = GAME4_SUCCESS;
        stateStart = millis();
      }
      else
      {
        Serial.println(" - Incorrect");
        firstTry = false;
        gameState = GAME4_WRONG;
        stateStart = millis();
        rgb.setColor(255, 0, 0);
        buzzer.playTone(TONE_ERROR_FREQ, TONE_ERROR_DURATION);
      }
      delay(100);
    }
    break;
  }
  case GAME4_WRONG:
  {
    lcd.updateLCD("Wrong Answer!", "Try Again...");
    if (millis() - stateStart >= WRONG_FEEDBACK_DURATION)
    {
      rgb.setColor(0, 0, 255);
      gameState = GAME4_WAIT_FOR_ANSWER;
      stateStart = millis();
    }
    break;
  }
  case GAME4_SUCCESS:
  {
    lcd.updateLCD("Correct!", "");
    rgb.setColor(0, 255, 0);
    buzzer.playSuccessMelody();
    if (millis() - stateStart >= GAME4_FEEDBACK_DURATION)
    {
      currentQuestion++;
      if (currentQuestion >= NUM_QUESTIONS)
      {
        gameState = GAME4_COMPLETE;
      }
      else
      {
        Serial.print("Question ");
        Serial.println(currentQuestion + 1);
        typewriterInit = false;
        firstTry = true;
        gameState = GAME4_SHOW_QUESTION;
      }
      stateStart = millis();
    }
    break;
  }
  case GAME4_COMPLETE:
  {
    noTone(PIN_BUZZER);
    if (!finalPrinted)
    {
      char finalLine[17];
      snprintf(finalLine, 17, "Good job %d/%d", correctCount, NUM_QUESTIONS);
      Serial.println("Final level complete. Game over!");
      rgb.setColor(0, 224, 0);
      buzzer.playSuccessMelody();
      Serial.print("Button presses: ");
      Serial.println(currentGamePresses);
      keyLed.printTimeUsed(game4StartTime);
      unsigned long timeTaken = millis() - game4StartTime;
      Score game4Score(4, currentGamePresses, timeTaken);
      game4FinalScore = game4Score.points;
      Serial.print("Game 4 Score: ");
      Serial.println(game4Score.points);
      lcd.updateLCD("Ohh, good job!", finalLine);
      finalPrinted = true;
      stateStart = millis();
    }
    if (millis() - stateStart >= GAME4_FEEDBACK_DURATION)
    {
      return true;
    }
    break;
  }
  case GAME4_TIME_UP:
  {
    return true;
  }
  }
  return false;
}