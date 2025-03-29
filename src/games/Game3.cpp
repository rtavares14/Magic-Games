#include "Game3.h"
#include <Arduino.h>
#include "LCD.h"
#include "KeyLed.h"
#include "RGBLed.h"
#include "Buzzer.h"
#include "Button.h"
#include "pins.h"

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

    // Target color values (multiples of 10)
    static int targetRed = 0, targetGreen = 0, targetBlue = 0;
    // User guess values
    static int guessRed = 0, guessGreen = 0, guessBlue = 0;
    // Current active channel: 0 = Red, 1 = Green, 2 = Blue
    static int currentChannel = 0;

    // Elapsed time since current state's start.
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
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Welcome: LEVEL 3");
                lcd.setCursor(0, 1);
                lcd.print("Color Game");
                lastMsgIndex = msgIndex;
            }
        }
        else if (elapsed < 4000)
        {
            msgIndex = 1;
            if (msgIndex != lastMsgIndex)
            {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Get ready...");
                lcd.setCursor(0, 1);
                lcd.print("Time is ticking");
                lastMsgIndex = msgIndex;
            }
        }
        else
        {
            // After 4000ms, generate target color and move to next state.
            targetRed = (random(0, 26)) * 10; // 0 to 250
            targetGreen = (random(0, 26)) * 10;
            targetBlue = (random(0, 26)) * 10;
            char dbg[50];
            sprintf(dbg, "Color to mach : R:%03d G:%03d B:%03d", targetRed, targetGreen, targetBlue);
            Serial.println(dbg);
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
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Memorize this");
                lcd.setCursor(0, 1);
                lcd.print("color!");
                lastMsgIndex = msgIndex;
            }
        }
        else if (elapsed < 4000)
        {
            msgIndex = 1;
            if (msgIndex != lastMsgIndex)
            {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Press btn when");
                lcd.setCursor(0, 1);
                lcd.print("ready");
                lastMsgIndex = msgIndex;
            }
        }
        if (button.isPressed() || elapsed >= 4000)
        {
            rgb.setColor(0, 0, 0); // Hide the target color.
            stateStart = millis();
            gameState = GAME3_USER_GUESS;
            lastMsgIndex = -1;
            delay(200); // Debounce delay.
        }
        break;
    }
    case GAME3_USER_GUESS:
    {
        // Interactive state; update LCD continuously based on user input.
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

        // Read potentiometer and map to a discrete value.
        int potValue = analogRead(PIN_POT);
        int step = map(potValue, 0, 1023, 0, 53);
        int discreteValue = step * 5;

        if (currentChannel == 0)
            guessRed = discreteValue;
        else if (currentChannel == 1)
            guessGreen = discreteValue;
        else if (currentChannel == 2)
            guessBlue = discreteValue;

        // Update LED to show user's current guess.
        rgb.setColor(guessRed, guessGreen, guessBlue);

        // Update LCD display (clearing only if values change is less critical here).
        char line0[17];
        char line1[17];
        sprintf(line0, "R:%03d G:%03d", guessRed, guessGreen);
        sprintf(line1, "B:%03d  btn:Done", guessBlue);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(line0);
        lcd.setCursor(0, 1);
        lcd.print(line1);

        if (button.isPressed())
        {
            stateStart = millis();
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
            stateStart = millis();
            buzzer.playSuccessMelody();
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
        int msgIndex = 0; // Only one message for success.
        if (elapsed < 2000)
        {
            if (msgIndex != lastMsgIndex)
            {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Correct! Well");
                lcd.setCursor(0, 1);
                lcd.print("done!");
                rgb.setColor(guessRed, guessGreen, guessBlue);
                lastMsgIndex = msgIndex;
            }
        }
        else
        {
            return true; // End game after 2000 ms.
        }
        break;
    }
    case GAME3_FAIL:
    {
        static int lastMsgIndex = -1;
        int msgIndex = 0; // Only one failure message.
        if (elapsed < 2000)
        {
            if (msgIndex != lastMsgIndex)
            {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Wrong color!");
                lcd.setCursor(0, 1);
                lcd.print("Try again...");
                lastMsgIndex = msgIndex;
            }
        }
        else
        {
            // After 2000 ms, generate a new target color and restart.
            targetRed = (random(0, 26)) * 10;
            targetGreen = (random(0, 26)) * 10;
            targetBlue = (random(0, 26)) * 10;
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
    }
    return false; // Game3 is still in progress.
}
