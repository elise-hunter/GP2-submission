#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>

// Top-level application screen
enum class GameState { MENU, PLAY, PAUSED, EXIT };

// A collectible box that the player can run over to earn points
struct BoxObject
{
    int   lane;               // Which lane the box is in: 0=left, 1=center, 2=right
    float worldZ;             // Position along the track in world space
    bool  collected;          // True once the player has picked this up
    int   meshVariant;        // Which size of box to draw: 0=small, 1=medium, 2=large
    float collectAnimTimer;   // Counts down while the floating collect animation plays
};

// A heart pickup that restores one life when collected
struct OneUpObject
{
    int   lane;
    float worldZ;
    bool  collected;
    float collectAnimTimer;
};

// A floating text label that shows score gains and drifts upward before disappearing
struct ScorePopup
{
    std::string text;
    float       screenX, screenY;  // Position in screen pixels
    float       timeRemaining;     // Seconds before this popup disappears
    glm::vec4   color;             // RGBA color, alpha used for fade out
};

// The world X position for each of the three lanes
static const float LANE_POSITIONS[3] = { -4.0f, 0.0f, 4.0f };

// Everything the renderer needs to draw one frame
struct GameRenderState
{
    float deltaTime;
    float dayNightProgress;   // 0 = full day, 1 = full night; bounces back and forth as the train travels
    float hitFlashTimer;      // How many seconds the red hit flash has left
    float trainLeanAngle;     // How much the train tilts sideways when changing lanes
    float distanceTraveled;   // How far the train has moved forward in total
    float boxSpinAngle;       // The current rotation angle for spinning boxes
    float cameraShakeTimer;   // How many seconds of camera shake remain after a hit
    float gameSpeed;          // How fast the train is currently moving
    glm::vec3 trainPosition;
    bool  isGameOver;
    int   gameOverMenuSelection; // 0=Restart, 1=Main Menu, 2=Quit
    bool  isPaused;
    int   pauseMenuSelection;    // 0=Resume, 1=Main Menu, 2=Quit
    float doublePointsTimer;    // seconds remaining in the double-points window (0 = inactive)
    float rainIntensity;        // 0 = no rain, 1 = full rain (smoothly transitions)
    int   totalSessions;        // lifetime game-over count loaded from stats.txt
    float totalDistanceAllTime; // lifetime distance in world units loaded from stats.txt
    int   score;
    int   lives;
    int   maxLives;
    int   combo;
    int   highScore;
    const std::vector<BoxObject>*  activeBoxes;
    const std::vector<BoxObject>*  activeRocks;
    const std::vector<OneUpObject>* activeOneUps;
    std::vector<ScorePopup>*        scorePopups;  // Non-const so timers can be updated during rendering
};
