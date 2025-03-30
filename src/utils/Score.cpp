#include "Score.h"

Score::Score() : gameId(0), buttonPresses(0), timeTaken(0), points(0) {}

Score::Score(int id, int presses, unsigned long time)
    : gameId(id), buttonPresses(presses), timeTaken(time) {
  calculatePoints(presses,time);
}

void Score::calculatePoints(int presses, unsigned long time) {
  unsigned long seconds = time / 1000; // convert milliseconds to seconds
  points = seconds * presses;
}
