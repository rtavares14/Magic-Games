#include "Score.h"

Score::Score() : gameId(0), buttonPresses(0), timeTaken(0), points(0) {}

Score::Score(int id, int presses, unsigned long time)
    : gameId(id), buttonPresses(presses), timeTaken(time) {
  calculatePoints();
}

void Score::calculatePoints() {
  int base = 1000;
  int minPresses;
  unsigned long idealTime;
  
  // Set ideal values for each game.
  // 3000 milliseconds = 3 seconds
  switch (gameId) {
    case 1:
      minPresses = 3;
      idealTime = 3000;   
      break;
    case 2:
      minPresses = 1;     
      idealTime = 5000;   
      break;
    case 3:
      minPresses = 1;    
      idealTime = 4000;   
      break;
  }
  
  // Calculate penalties for extra button presses and extra time.
  int pressPenalty = (buttonPresses > minPresses) ? (buttonPresses - minPresses) * 100 : 0;
  int timePenalty = (timeTaken > idealTime) ? ((timeTaken - idealTime) / 1000) * 50 : 0;
  
  points = base - pressPenalty - timePenalty;
  if (points < 0) {
    points = 0;
  }
}
