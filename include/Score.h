#ifndef SCORE_H
#define SCORE_H

#include <Arduino.h>

class Score {
  public:
    int gameId;             
    int buttonPresses;       
    unsigned long timeTaken;
    int points;            

    Score();
    Score(int id, int presses, unsigned long time);
    void calculatePoints(int presses, unsigned long time);
};

#endif
