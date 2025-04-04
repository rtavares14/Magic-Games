#include "Game3.h"
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

//-----------------------
// Constant Definitions
//-----------------------

// Durations (in milliseconds)
const unsigned long GAME3_INIT_PHASE1_DURATION = 2000;
const unsigned long GAME3_INIT_PHASE2_DURATION = 4000;
const unsigned long GAME3_SHOW_COLOR_PHASE1_DURATION = 2000;
const unsigned long GAME3_SHOW_COLOR_PHASE2_DURATION = 4000;
const unsigned long GAME3_DEBOUNCE_DELAY = 200;
const unsigned long GAME3_LCD_UPDATE_INTERVAL = 500;
const unsigned long GAME3_SUCCESS_PHASE_INTERVAL = 2000;
const unsigned long GAME3_SUCCESS_DISPLAY_DURATION = 6000;
const unsigned long GAME3_FAIL_DURATION = 2000;

// Color tolerance for validation
const int GAME3_COLOR_TOLERANCE = 15;

// Potentiometer mapping constants
const int GAME3_POT_MAX_STEP = 9;
const int GAME3_DISCRETE_VALUE_MULTIPLIER = 32;

// Define game states.
enum Game3State
{
    GAME3_INIT,
    GAME3_SHOW_COLOR,
    GAME3_USER_GUESS,
    GAME3_VALIDATE,
    GAME3_SUCCESS,
    GAME3_FAIL
};

bool updateGame3()
{
    static uint32_t game3StartTime = 0;

    static Game3State gameState = GAME3_INIT;
    static unsigned long stateStart = millis();

    // Target and guess color components.
    static int targetRed = 0, targetGreen = 0, targetBlue = 0;
    static int guessRed = 0, guessGreen = 0, guessBlue = 0;
    // Active channel: 0 = Red, 1 = Green, 2 = Blue.
    static int currentChannel = 0;

    unsigned long elapsed = millis() - stateStart;

    switch (gameState)
    {
    case GAME3_INIT:
    {
        static int lastMsgIndex = -1;
        int msgIndex = -1;
        if (elapsed < GAME3_INIT_PHASE1_DURATION)
        {
            msgIndex = 0;
            if (msgIndex != lastMsgIndex)
            {
                lcd.lcdShow("Welcome: LEVEL 3", "Color Game");
                lastMsgIndex = msgIndex;
            }
        }
        else if (elapsed < GAME3_INIT_PHASE2_DURATION)
        {
            msgIndex = 1;
            if (msgIndex != lastMsgIndex)
            {
                lcd.lcdShow("Get ready...", "Time is ticking");
                lastMsgIndex = msgIndex;
            }
        }
        else
        {
            // After init phase, generate target color.
            rgb.getRandomColor(targetRed, targetGreen, targetBlue);
            {
                char dbg[50];
                Serial.println("------------------------------------");
                Serial.println("---------------Game-3---------------");
                Serial.println("Color generated!");
                sprintf(dbg, "Color to match: R:%03d G:%03d B:%03d", targetRed, targetGreen, targetBlue);
                Serial.println(dbg);
            }
            // Reset user's guess.
            guessRed = 0;
            guessGreen = 0;
            guessBlue = 0;
            currentChannel = 0;
            stateStart = millis();
            gameState = GAME3_SHOW_COLOR;
            lastMsgIndex = -1;
        }
        break;
    }
    case GAME3_SHOW_COLOR:
    {
        static int lastMsgIndex = -1;
        int msgIndex = -1;
        // Display target color.
        rgb.setColor(targetRed, targetGreen, targetBlue);
        if (elapsed < GAME3_SHOW_COLOR_PHASE1_DURATION)
        {
            msgIndex = 0;
            if (msgIndex != lastMsgIndex)
            {
                lcd.lcdShow("Memorize this", "color!");
                lastMsgIndex = msgIndex;
            }
        }
        else if (elapsed < GAME3_SHOW_COLOR_PHASE2_DURATION)
        {
            msgIndex = 1;
            if (msgIndex != lastMsgIndex)
            {
                lcd.lcdShow("Press btn when", "ready");
                lastMsgIndex = msgIndex;
            }
        }
        if (button.isPressed() || elapsed >= GAME3_SHOW_COLOR_PHASE2_DURATION)
        {
            // Hide the target color.
            rgb.setColor(0, 0, 0);
            stateStart = millis();
            gameState = GAME3_USER_GUESS;
            lastMsgIndex = -1;
            // Initialize guessing phase timer.
            game3StartTime = millis();
            delay(GAME3_DEBOUNCE_DELAY); // Debounce delay.
        }
        break;
    }
    case GAME3_USER_GUESS:
    {
        // Process key input for channel selection and reset.
        uint8_t keys = keyLed.readButtons();
        if (keys & 0x80)
        { // Reset guess.
            guessRed = 0;
            guessGreen = 0;
            guessBlue = 0;
            currentChannel = 0;
            delay(GAME3_DEBOUNCE_DELAY);
        }
        else if (keys & 0x01)
        { // Key 1 selects Red.
            currentChannel = 0;
            delay(GAME3_DEBOUNCE_DELAY);
        }
        else if (keys & 0x02)
        { // Key 2 selects Green.
            currentChannel = 1;
            delay(GAME3_DEBOUNCE_DELAY);
        }
        else if (keys & 0x04)
        { // Key 3 selects Blue.
            currentChannel = 2;
            delay(GAME3_DEBOUNCE_DELAY);
        }

        // Read potentiometer and update the active channel.
        int potValue = analogRead(PIN_POT);
        int step = map(potValue, 0, 1023, 0, GAME3_POT_MAX_STEP);
        int discreteValue = step * GAME3_DISCRETE_VALUE_MULTIPLIER;
        if (currentChannel == 0)
            guessRed = discreteValue;
        else if (currentChannel == 1)
            guessGreen = discreteValue;
        else if (currentChannel == 2)
            guessBlue = discreteValue;

        // Update LED with user's current guess.
        rgb.setColor(guessRed, guessGreen, guessBlue);

        // Update the LCD every GAME3_LCD_UPDATE_INTERVAL.
        static unsigned long lastDisplayUpdate = 0;
        if (millis() - lastDisplayUpdate >= GAME3_LCD_UPDATE_INTERVAL)
        {
            char newLine0[17];
            char newLine1[17];
            sprintf(newLine0, "R:%03d G:%03d", guessRed, guessGreen);
            sprintf(newLine1, "B:%03d  btn:Done", guessBlue);
            static char lastLine0[17] = "";
            static char lastLine1[17] = "";
            if (strcmp(newLine0, lastLine0) != 0 || strcmp(newLine1, lastLine1) != 0)
            {
                lcd.lcdShow(newLine0, newLine1);
                strcpy(lastLine0, newLine0);
                strcpy(lastLine1, newLine1);
            }
            lastDisplayUpdate = millis();
        }

        if (button.isPressed())
        {
            // On guess submission, proceed to validation.
            gameState = GAME3_VALIDATE;
            delay(GAME3_DEBOUNCE_DELAY);
        }
        break;
    }
    case GAME3_VALIDATE:
    {
        // Debug output for user's guess.
        char dbg[50];
        sprintf(dbg, "Color sent: R:%03d G:%03d B:%03d", guessRed, guessGreen, guessBlue);
        Serial.println(dbg);

        bool redOk = abs(guessRed - targetRed) <= GAME3_COLOR_TOLERANCE;
        bool greenOk = abs(guessGreen - targetGreen) <= GAME3_COLOR_TOLERANCE;
        bool blueOk = abs(guessBlue - targetBlue) <= GAME3_COLOR_TOLERANCE;

        if (redOk && greenOk && blueOk)
        {
            Serial.println("Right color, congrats");
            gameState = GAME3_SUCCESS;
        }
        else
        {
            Serial.println("Wrong Color");
            gameState = GAME3_FAIL;
            stateStart = millis();
            buzzer.playErrorTone();
        }
        break;
    }
    case GAME3_SUCCESS:
    {
        static int lastMsgIndex = -1;
        int phase = elapsed / GAME3_SUCCESS_PHASE_INTERVAL;
        if (phase != lastMsgIndex)
        {
            lcd.clear();
            lastMsgIndex = phase;
        }

        static bool winningTriggered = false;
        if (elapsed >= GAME3_SUCCESS_DISPLAY_DURATION && !winningTriggered)
        {
            lcd.lcdShow("You have a good", "eyes for this");
            rgb.setColor(0, 255, 0); // Green light for success.
            buzzer.playSuccessMelody();
            keyLed.printTimeUsed(game3StartTime);
            Serial.print("Button presses: ");
            Serial.println(currentGamePresses);
            unsigned long timeTaken = millis() - game3StartTime;
            Score game3Score(3, currentGamePresses, timeTaken);
            game3FinalScore = game3Score.points;
            Serial.print("Game 3 Score: ");
            Serial.println(game3Score.points);
            winningTriggered = true;
        }
        if (elapsed >= GAME3_SUCCESS_DISPLAY_DURATION)
        {
            return true; // End Game 3.
        }
        break;
    }
    case GAME3_FAIL:
    {
        static int lastMsgIndex = -1;
        int msgIndex = 0; // Single failure message.
        if (elapsed < GAME3_FAIL_DURATION)
        {
            if (msgIndex != lastMsgIndex)
            {
                lcd.lcdShow("Wrong color!", "Try again...");
                lastMsgIndex = msgIndex;
            }
        }
        else
        {
            // After failure duration, generate a new target color and restart.
            rgb.getRandomColor(targetRed, targetGreen, targetBlue);
            // Reset user's guess and active channel.
            guessRed = 0;
            guessGreen = 0;
            guessBlue = 0;
            currentChannel = 0;
            stateStart = millis();
            gameState = GAME3_SHOW_COLOR;
            lastMsgIndex = -1;
            char dbg[50];
            sprintf(dbg, "Color to match: R:%03d G:%03d B:%03d", targetRed, targetGreen, targetBlue);
            Serial.println(dbg);
        }
        break;
    }
    }
    return false; // Game 3 is still in progress.
}
