#include "Button.h"

Button::Button(uint8_t pin, unsigned long debounceDelay)
    : pin(pin), debounceDelay(debounceDelay), stableState(HIGH), lastRawState(HIGH), lastDebounceTime(0), fallingEdgeDetected(false) {}

void Button::begin()
{
    pinMode(pin, INPUT_PULLUP);
    stableState = digitalRead(pin);
    lastRawState = stableState;
}

void Button::update()
{
    int rawState = digitalRead(pin);
    if (rawState != lastRawState)
    {
        lastDebounceTime = millis();
        lastRawState = rawState;
    }
    if ((millis() - lastDebounceTime) > debounceDelay)
    {
        if (rawState != stableState)
        {
            if (stableState == HIGH && rawState == LOW)
            {
                fallingEdgeDetected = true;
            }
            else
            {
                fallingEdgeDetected = false;
            }
            stableState = rawState;
        }
        else
        {
            fallingEdgeDetected = false;
        }
    }
}

bool Button::isPressed()
{
    return fallingEdgeDetected;
}
