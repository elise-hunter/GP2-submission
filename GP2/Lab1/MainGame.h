#pragma once
#include <SDL/SDL.h>
#include <vector>
#include "Data.h"
#include "Display.h"
#include "Audio.h"
#include "Renderer.h"

// Top level game class. Owns all subsystems, runs the game loop,
// and coordinates input, logic, and rendering each frame.
class MainGame
{
public:
    MainGame();
    ~MainGame();

    // Sets up all subsystems and enters the game loop.
    void run();

private:
    // Top level flow
    void initSystems();
    void gameLoop();
    void processInput();
    void updateGame(float deltaTime);
    void resetGame();

    // Update helpers called once per frame from updateGame
    float calculateGameSpeed() const;
    void  updateTimers(float deltaTime);
    void  updateCollectAnimTimers(float deltaTime);
    void  trySpawnBox(float deltaTime);
    void  processBoxCollections();
    void  trySpawnRock(float deltaTime);
    void  processRockCollisions();
    void  trySpawnOneUp(float deltaTime);
    void  processOneUpCollections();
    void  handleGameOver();
    void  updateDoublePoints(float deltaTime);
    void  updateRain(float deltaTime);
    void  checkMilestones();

    // Save and load helpers
    void loadHighScore();
    void saveHighScore();
    void loadStats();
    void saveStats();

    // Subsystems
    Display      gameDisplay;
    GameState    gameState;
    AudioSystem  audioSystem;
    GameRenderer renderer;

    // Active world objects
    std::vector<BoxObject>   activeBoxes;
    std::vector<BoxObject>   activeRocks;
    std::vector<OneUpObject> activeOneUps;
    std::vector<ScorePopup>  scorePopups;

    // Train position and movement
    int   currentLane;      // Target lane: 0 = left, 1 = centre, 2 = right
    float trainWorldX;      // Smoothed world X position
    float trainLeanAngle;   // Z axis tilt applied while changing lanes, in radians

    // Menu and pause state
    float menuTime;               // Seconds elapsed since the menu was first shown
    int   pauseMenuSelection;     // Highlighted pause option: 0 = Resume, 1 = Main Menu, 2 = Quit
    int   gameOverMenuSelection;  // Highlighted game over option: 0 = Restart, 1 = Main Menu, 2 = Quit

    // Double points
    float doublePointsTimer;    // Seconds remaining in the active window (0 means inactive)
    float doublePointsCooldown; // Seconds until the next window triggers

    // Rain
    float rainIntensity;        // Current smoothed intensity between 0 and 1
    float rainTargetIntensity;  // Target value that rainIntensity is moving towards
    float rainTimer;            // Seconds of rain remaining
    float rainCooldown;         // Seconds until the next rain starts

    // Milestone tracking
    int nextMilestoneIndex;

    // Lifetime stats loaded from disk
    int   totalSessions;
    float totalDistanceAllTime;

    // Gameplay progress
    float distanceTraveled;
    float boxSpinAngle;  // Accumulated Y rotation for spinning collectible boxes

    // Score and lives
    int  score;
    int  lives;
    int  maxLives;
    int  combo;
    int  highScore;
    bool isGameOver;
    bool hasHighScoreBeenSaved;
    bool hasShownMaxSpeedWarning;

    // Spawn timers
    float boxSpawnTimer;
    float rockSpawnTimer;
    float oneUpSpawnTimer;

    // Visual feedback timers
    float hitFlashTimer;    // How long the red screen flash has left after hitting a rock
    float cameraShakeTimer; // How long the camera shake has left after hitting a rock

    unsigned int lastFrameTime;  // SDL tick count at the start of the previous frame
};
