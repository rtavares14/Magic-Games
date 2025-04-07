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
struct TriviaQuestion {
  const char* question;
  const char* options[5];  // Options labeled A–E
  int correctIndex;        // 0-based index: 0=A, 1=B, …, 4=E
};

// Updated general knowledge questions (each 16 chars or less).
static const TriviaQuestion questions[] = {
  {
    "France: Capital",
    {"Paris", "Berlin", "Madrid", "Rome", "Lisbon"},
    0
  },
  {
    "Largest planet?",
    {"Earth", "Mars", "Jupiter", "Saturn", "Neptune"},
    2
  },
  {
    "Hamlet Author?",
    {"Dickens", "Shakespeare", "Twain", "Tolstoy", "Austen"},
    1
  },
  {
    "Water Boils at?",
    {"90°C", "100°C", "110°C", "120°C", "80°C"},
    1
  },
  {
    "Element 'O' is?",
    {"Gold", "Oxygen", "Silver", "Iron", "Hydrogen"},
    1
  },
  {
    "Largest ocean?",
    {"Atlantic", "Indian", "Arctic", "Southern", "Pacific"},
    4
  },
  {
    "Portugal is in?",
    {"Africa", "Asia", "S.America", "Europe", "Australia"},
    3
  }
};

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
enum Game4State {
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

bool updateGame4() {
  static Game4State gameState = GAME4_INIT;
  static unsigned long stateStart = millis();
  static int currentQuestion = 0;  
  static int selectedOption = 0; 
  static uint32_t game4StartTime = 0;
  static bool debugPrinted = false;

  static int charIndex = 0;            
  static unsigned long lastCharTime = 0; 
  static char typedQuestion[17] = {0};   // Buffer for gradually built question (max 16 chars)
  static bool typewriterInit = false;    // Initialize typewriter state only once per question

  static unsigned long lastOptionUpdate = 0;

  // Check global timer expiration.
  uint32_t elapsedGlobal = millis() - globalStartTime;
  if (elapsedGlobal >= TOTAL_TIME) {
    return true;
  }

  switch (gameState) {
    case GAME4_INIT: {
      // Set the LED to blue at the start.
      rgb.setColor(0, 0, 255);
      if (!debugPrinted) {
        Serial.println("Game 4");
        debugPrinted = true;
      }
      // Display welcome message.
      if (millis() - stateStart < GAME4_INIT_DURATION) {
        lcd.updateLCD("Trivia Time!", "Get Ready...");
      } else {
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
    case GAME4_SHOW_QUESTION: {
      // Use a non-blocking typewriter effect to print the question gradually.
      const char* fullQuestion = questions[currentQuestion].question;
      char truncatedQuestion[17];
      strncpy(truncatedQuestion, fullQuestion, 16);
      truncatedQuestion[16] = '\0';
      int len = strlen(truncatedQuestion);
      if (!typewriterInit) {
        charIndex = 0;
        lastCharTime = millis();
        memset(typedQuestion, 0, sizeof(typedQuestion));
        typewriterInit = true;
      }
      if (charIndex < len) {
        if (millis() - lastCharTime >= TYPEWRITER_DELAY) {
          typedQuestion[charIndex] = truncatedQuestion[charIndex];
          charIndex++;
          lastCharTime = millis();
        }
        // While typing, show the gradually built question on line 1.
        // Line 2 shows default option "A".
        char optionLine[17];
        snprintf(optionLine, 17, "Select: A");
        lcd.updateLCD(typedQuestion, optionLine);
      } else {
        // Finished printing question text.
        Serial.print("Q");
        Serial.print(currentQuestion + 1);
        Serial.print(": ");
        Serial.println(truncatedQuestion);
        gameState = GAME4_WAIT_FOR_ANSWER;
        stateStart = millis();
        // Initialize the option update timer.
        lastOptionUpdate = millis();
      }
      break;
    }
    case GAME4_WAIT_FOR_ANSWER: {
      // Ensure the LED stays blue while waiting.
      rgb.setColor(0, 0, 255);
      
      // Update option display at regular intervals.
      if (millis() - lastOptionUpdate >= GAME4_OPTION_UPDATE_INTERVAL) {
        int potValue = analogRead(PIN_POT);
        // Map pot value (0–1023) to an option index between 0 and 4.
        int mappedOption = map(potValue, 0, 1023, 0, 5);
        if (mappedOption > 4) mappedOption = 4;
        selectedOption = mappedOption;

        // Update LCD: line 1 shows full (truncated) question, line 2 shows selected option.
        char questionLine[17];
        strncpy(questionLine, questions[currentQuestion].question, 16);
        questionLine[16] = '\0';
        char optionLine[17];
        snprintf(optionLine, 17, "%c: %s", 'A' + selectedOption, questions[currentQuestion].options[selectedOption]);
        //lcd.updateLCD(questionLine, optionLine);
        lastOptionUpdate = millis();
      }

      // When the button is pressed, check the answer.
      if (button.isPressed()) {
        Serial.print("Question ");
        Serial.print(currentQuestion + 1);
        Serial.print(" Answer: ");
        Serial.print((char)('A' + selectedOption));
        if (selectedOption == questions[currentQuestion].correctIndex) {
          Serial.println(" - Correct");
          // If correct on first try, increment the counter.
          if (firstTry) {
            correctCount++;
          }
          // Transition to success state.
          gameState = GAME4_SUCCESS;
          stateStart = millis();
        } else {
          Serial.println(" - Incorrect");
          // Mark that this question is no longer answered on the first try.
          firstTry = false;
          // Transition to wrong feedback state.
          gameState = GAME4_WRONG;
          stateStart = millis();
          // Set LED to red.
          rgb.setColor(255, 0, 0);
          // Play error tone (blocking call).
          buzzer.playTone(TONE_ERROR_FREQ, TONE_ERROR_DURATION);
        }
        // Debounce delay.
        delay(100);
      }
      break;
    }
    case GAME4_WRONG: {
      // In wrong state, show feedback message.
      lcd.updateLCD("Wrong Answer!", "Try Again...");
      // Stay in this state for WRONG_FEEDBACK_DURATION.
      if (millis() - stateStart >= WRONG_FEEDBACK_DURATION) {
        // Revert LED to blue.
        rgb.setColor(0, 0, 255);
        // Return to waiting for answer.
        gameState = GAME4_WAIT_FOR_ANSWER;
        stateStart = millis();
      }
      break;
    }
    case GAME4_SUCCESS: {
      // In success state, show correct feedback.
      lcd.updateLCD("Correct!", "");
      // Set LED to green.
      rgb.setColor(0, 255, 0);
      // Play success melody (blocking call).
      buzzer.playSuccessMelody();
      // After feedback duration, advance to the next question.
      if (millis() - stateStart >= GAME4_FEEDBACK_DURATION) {
        currentQuestion++;
        if (currentQuestion >= NUM_QUESTIONS) {
          gameState = GAME4_COMPLETE;
        } else {
          Serial.print("Question ");
          Serial.println(currentQuestion + 1);
          // Prepare for next question.
          typewriterInit = false;
          firstTry = true;
          gameState = GAME4_SHOW_QUESTION;
        }
        stateStart = millis();
      }
      break;
    }
    case GAME4_COMPLETE: {
      // Stop any tone playing.
      noTone(PIN_BUZZER);
      // Final message: show total correct answers out of total questions.
      char finalLine[17];
      snprintf(finalLine, 17, "Good job %d/%d", correctCount, NUM_QUESTIONS);
      lcd.updateLCD("Ohh, good job!", finalLine);
      // Remain here for a while before ending the game.
      if (millis() - stateStart >= GAME4_FEEDBACK_DURATION) {
        return true;
      }
      break;
    }
    case GAME4_TIME_UP: {
      return true;
    }
  }
  return false;
}
