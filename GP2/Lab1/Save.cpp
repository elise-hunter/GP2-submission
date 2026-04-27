// Reads and writes the high score and lifetime stats files.
// Both files live in the working directory next to the executable.

#include "MainGame.h"
#include <fstream>
#include <string>

// Reads the saved high score from disk.
// Does nothing if the file does not exist yet.
void MainGame::loadHighScore()
{
    std::ifstream file("highscore.txt");
    if (file.is_open()) file >> highScore;
}

// Writes the current high score to disk so it persists between sessions.
void MainGame::saveHighScore()
{
    std::ofstream file("highscore.txt");
    if (file.is_open()) file << highScore;
}

// Reads the lifetime stats (total sessions and total distance) from disk.
// Does nothing if the file does not exist yet.
void MainGame::loadStats()
{
    std::ifstream file("stats.txt");
    if (file.is_open()) file >> totalSessions >> totalDistanceAllTime;
}

// Writes the lifetime stats to disk so they persist between sessions.
void MainGame::saveStats()
{
    std::ofstream file("stats.txt");
    if (file.is_open()) file << totalSessions << " " << totalDistanceAllTime;
}
