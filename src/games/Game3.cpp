#include "Game3.h"
#include <Arduino.h>
#include "LCD.h"
#include "KeyLed.h"
#include "RGBLed.h"
#include "Buzzer.h"
#include "Button.h"
#include "pins.h"
#include "Score.h"

//global timer defined in main.cpp.
extern uint32_t globalStartTime;
extern const uint32_t TOTAL_TIME;
extern int currentGamePresses;
static uint32_t game3StartTime = 0;
extern int game3FinalScore;

extern LCD lcd;
extern KeyLed keyLed;
extern RGBLed rgb;
extern Buzzer buzzer;
extern Button button;

bool updateGame3()
{
    enum Game3State
    {
        GAME3_INIT,
        GAME3_SHOW_COLOR,
        GAME3_USER_GUESS,
        GAME3_VALIDATE,
        GAME3_SUCCESS,
        GAME3_FAIL
    };
    static Game3State gameState = GAME3_INIT;
    static unsigned long stateStart = millis();

    static int targetRed = 0, targetGreen = 0, targetBlue = 0;
    static int guessRed = 0, guessGreen = 0, guessBlue = 0;
    // Current active channel: 0 = Red, 1 = Green, 2 = Blue
    static int currentChannel = 0;

    unsigned long elapsed = millis() - stateStart;

    switch (gameState)
    {
    case GAME3_INIT:
    {
        static int lastMsgIndex = -1;
        int msgIndex = -1;
        if (elapsed < 2000)
        {
            msgIndex = 0;
            if (msgIndex != lastMsgIndex)
            {
                lcd.lcdShow("Welcome: LEVEL 3", "Color Game");
                lastMsgIndex = msgIndex;
            }
        }
        else if (elapsed < 4000)
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
            // After 4000ms, generate target color and transition.
            rgb.getRandomColor(targetRed, targetGreen, targetBlue);
            {
                rgb.getRandomColor(targetRed, targetGreen, targetBlue);
                char dbg[50];
                Serial.println("------------------------------------");
                Serial.println("---------------Game-3---------------");
                Serial.println("Colot generated!");
                sprintf(dbg, "Color to mach : R:%03d G:%03d B:%03d", targetRed, targetGreen, targetBlue);
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
        // LED displays the target color.
        rgb.setColor(targetRed, targetGreen, targetBlue);
        if (elapsed < 2000)
        {
            msgIndex = 0;
            if (msgIndex != lastMsgIndex)
            {
                lcd.lcdShow("Memorize this", "color!");
                lastMsgIndex = msgIndex;
            }
        }
        else if (elapsed < 4000)
        {
            msgIndex = 1;
            if (msgIndex != lastMsgIndex)
            {
                lcd.lcdShow("Press btn when", "ready");
                lastMsgIndex = msgIndex;
            }
        }
        if (button.isPressed() || elapsed >= 4000)
        {
            rgb.setColor(0, 0, 0); // Hide the target color.
            stateStart = millis();
            gameState = GAME3_USER_GUESS;
            lastMsgIndex = -1;
            // Initialize game stats for the guessing phase.
            game3StartTime = millis();

            delay(200); // Debounce delay.
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
            delay(200);
        }
        else if (keys & 0x01)
        { // Key 1 selects Red.
            currentChannel = 0;
            delay(200);
        }
        else if (keys & 0x02)
        { // Key 2 selects Green.
            currentChannel = 1;
            delay(200);
        }
        else if (keys & 0x04)
        { // Key 3 selects Blue.
            currentChannel = 2;
            delay(200);
        }

        // Read potentiometer and update only the active channel.
        int potValue = analogRead(PIN_POT);
        int step = map(potValue, 0, 1023, -1, 26);
        int discreteValue = step * 10;
        if (currentChannel == 0)
            guessRed = discreteValue;
        else if (currentChannel == 1)
            guessGreen = discreteValue;
        else if (currentChannel == 2)
            guessBlue = discreteValue;

        // Update LED to show user's current guess.
        rgb.setColor(guessRed, guessGreen, guessBlue);

        // Update the LCD every 500 ms.
        static unsigned long lastDisplayUpdate = 0;
        if (millis() - lastDisplayUpdate >= 500)
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
            // Increment attempt counter each time a guess is submitted.
            gameState = GAME3_VALIDATE;
            delay(200);
        }
        break;
    }
    case GAME3_VALIDATE:
    {
        // Print debug information.
        char dbg[50];
        sprintf(dbg, "Color sent:  R:%03d G:%03d B:%03d", guessRed, guessGreen, guessBlue);
        Serial.println(dbg);

        bool redOk = abs(guessRed - targetRed) <= 15;
        bool greenOk = abs(guessGreen - targetGreen) <= 15;
        bool blueOk = abs(guessBlue - targetBlue) <= 15;

        if (redOk && greenOk && blueOk)
        {
            Serial.println("Right color congrats");
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
        int phase = elapsed / 2000;
        if (phase != lastMsgIndex)
        {
            lcd.clear();
            lastMsgIndex = phase;
        }

        static bool winningTriggered = false;
        if (elapsed >= 6000 && !winningTriggered)
        {
            lcd.lcdShow("You have a good", "eyes for this");
            // Play winning sound and display green light.
            rgb.setColor(0, 255, 0);
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
        if (elapsed >= 6000)
        {
            return true; // End game after 6000ms.
        }
        break;
    }
    case GAME3_FAIL:
    {
        static int lastMsgIndex = -1;
        int msgIndex = 0; // Single failure message.
        if (elapsed < 2000)
        {
            if (msgIndex != lastMsgIndex)
            {
                lcd.lcdShow("Wrong color!", "Try again...");
                lastMsgIndex = msgIndex;
            }
        }
        else
        {
            // After 2000ms, generate a new target color and restart.
            rgb.getRandomColor(targetRed, targetGreen, targetBlue);
            guessRed, guessGreen, guessBlue, currentChannel = 0;
            stateStart = millis();
            gameState = GAME3_SHOW_COLOR;
            lastMsgIndex = -1;
            char dbg[50];
            sprintf(dbg, "Color to mach : R:%03d G:%03d B:%03d", targetRed, targetGreen, targetBlue);
            Serial.println(dbg);
        }
        break;
    }
    }
    return false; // Game3 is still in progress.
}
