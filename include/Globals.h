#ifndef GLOBALS_H
#define GLOBALS_H

// Declare global objects from main.cpp.
extern LCD lcd;
extern KeyLed keyLed;
extern RGBLed rgb;
extern Buzzer buzzer;
extern Button button;

// Use the global timer defined in main.cpp.
extern uint32_t globalStartTime;
extern const uint32_t TOTAL_TIME;

// Declare the global game press counter.
extern int currentGamePresses;
extern int game1FinalScore;
extern int game2FinalScore;
extern int game3FinalScore;
extern int game4FinalScore;

#endif