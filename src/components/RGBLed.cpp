#include "RGBLed.h"
#include "pins.h"

RGBLed::RGBLed() {}

void RGBLed::begin()
{
    pinMode(PIN_RED, OUTPUT);
    pinMode(PIN_GREEN, OUTPUT);
    pinMode(PIN_BLUE, OUTPUT);
}

void RGBLed::setColor(uint8_t r, uint8_t g, uint8_t b)
{
    analogWrite(PIN_RED, r);
    analogWrite(PIN_GREEN, g);
    analogWrite(PIN_BLUE, b);
}

void RGBLed::loadingEffect(unsigned long duration)
{
    unsigned long startTime = millis();
    // set of colors for a rainbow effect.
    const int numColors = 8;
    uint8_t colors[numColors][3] = {
        {255, 0, 0},    // Red
        {255, 127, 0},  // Orange
        {255, 255, 0},  // Yellow
        {0, 255, 0},    // Green
        {0, 0, 255},    // Blue
        {75, 0, 130},   // Indigo
        {148, 0, 211},  // Violet
        {255, 255, 255} // White
    };

    int currentColor = 0;
    while (millis() - startTime < duration)
    {
        setColor(colors[currentColor][0], colors[currentColor][1], colors[currentColor][2]);
        delay(100);
        currentColor = (currentColor + 1) % numColors;
    }
}

void RGBLed::blink(uint8_t r, uint8_t g, uint8_t b, int delayTime)
{
    setColor(r, g, b);
    delay(delayTime);
    setColor(0, 0, 0);
    delay(delayTime);
}

void RGBLed::loadingAnimation(uint32_t elapsed)
{
    // Cycle time in milliseconds (here, 800ms for a full cycle)
    uint32_t cycleTime = elapsed % 800;

    if (cycleTime < 100)
        setColor(255, 0, 0);
    else if (cycleTime < 200)
        setColor(255, 127, 0);
    else if (cycleTime < 300)
        setColor(255, 255, 0);
    else if (cycleTime < 400)
        setColor(0, 255, 0);
    else if (cycleTime < 500)
        setColor(0, 0, 255);
    else if (cycleTime < 600)
        setColor(75, 0, 130);
    else if (cycleTime < 700)
        setColor(148, 0, 211);
    else
        setColor(255, 255, 255);
}

void RGBLed::getRandomColor(int &red, int &green, int &blue)
{
    const int numColors = 9;
    int colors[numColors][3] = {
        {224, 160, 0},  // Orange
        {160, 0, 160},  // Purple
        {0, 224, 224},  // Cyan
        {224, 224, 0},  // Yellow
        {224, 0, 224},  // Magenta
        {224, 96, 192}, // Pink
        {0, 128, 128},  // Teal
        {128, 128, 0},  // Olive
        {224, 224, 0}   // Gold
    };
    int index = random(0, numColors);
    red = colors[index][0];
    green = colors[index][1];
    blue = colors[index][2];
}
