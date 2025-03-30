#ifndef SCORE_H
#define SCORE_H

#include <Arduino.h>

class Score {
  public:
    int gameId;              // Game identifier (e.g. 1, 2, 3)
    int buttonPresses;       // Number of button presses recorded during the game
    unsigned long timeTaken; // Time taken (in milliseconds) for the game
    int points;              // Computed score points

    Score();
    Score(int id, int presses, unsigned long time);
    void calculatePoints();
};

#endif
