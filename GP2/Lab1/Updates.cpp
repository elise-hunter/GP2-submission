// Per-frame update helpers: timers, speed, double points, rain, milestones, and game over.

#include "MainGame.h"
#include "Constants.h"
#include <glm/glm.hpp>
#include <cstdlib>
#include <cmath>
#include <string>

// Returns the current train speed in world units per second.
// Speed increases with distance but never goes above the cap.
float MainGame::calculateGameSpeed() const
{
    return glm::min(MinGameSpeed + distanceTraveled * SpeedAccelFactor, MaxGameSpeed);
}

// Counts down the hit flash, camera shake, and lane lean timers each frame.
void MainGame::updateTimers(float deltaTime)
{
    hitFlashTimer    = glm::max(hitFlashTimer    - deltaTime, 0.0f);
    cameraShakeTimer = glm::max(cameraShakeTimer - deltaTime, 0.0f);

    // Smoothly return the lean angle to zero after a lane change.
    trainLeanAngle = glm::mix(trainLeanAngle, 0.0f, glm::min(LeanSmoothingSpeed * deltaTime, 1.0f));
}

// Counts down the floating collect animation timer on boxes and one-ups that have been picked up.
void MainGame::updateCollectAnimTimers(float deltaTime)
{
    for (auto& box : activeBoxes)
        if (box.collected && box.collectAnimTimer > 0.0f)
            box.collectAnimTimer = glm::max(box.collectAnimTimer - deltaTime, 0.0f);

    for (auto& oneUp : activeOneUps)
        if (oneUp.collected && oneUp.collectAnimTimer > 0.0f)
            oneUp.collectAnimTimer = glm::max(oneUp.collectAnimTimer - deltaTime, 0.0f);
}

// Counts down the double points cooldown and activates a window when it expires.
// Once a window is active it counts that down instead; a new cooldown starts when it ends.
void MainGame::updateDoublePoints(float deltaTime)
{
    if (isGameOver) return;

    if (doublePointsTimer > 0.0f)
    {
        doublePointsTimer = glm::max(doublePointsTimer - deltaTime, 0.0f);
        return;
    }

    doublePointsCooldown -= deltaTime;
    if (doublePointsCooldown > 0.0f) return;

    // Start a new double points window and set the next cooldown.
    doublePointsTimer    = DoublePointsDuration;
    doublePointsCooldown = DoublePointsCooldownMin
        + static_cast<float>(std::rand() % static_cast<int>(DoublePointsCooldownRange * 100)) / 100.0f;

    audioSystem.playBeep(DoublePointsBeepFreq1, DoublePointsBeepDur, DoublePointsBeepVol);
    audioSystem.playBeep(DoublePointsBeepFreq2, DoublePointsBeepDur, DoublePointsBeepVol);

    ScorePopup popup;
    popup.timeRemaining = 1.6f;
    popup.screenX       = gameDisplay.getWidth()  * 0.5f;
    popup.screenY       = gameDisplay.getHeight() * 0.35f;
    popup.text          = "2X POINTS!";
    popup.color         = glm::vec4(0.2f, 1.0f, 0.85f, 1.0f);
    scorePopups.push_back(popup);
}

// Smoothly fades rain in and out on a random cooldown and duration cycle.
// Rain is turned off immediately when the game ends.
void MainGame::updateRain(float deltaTime)
{
    if (isGameOver)
        rainTargetIntensity = 0.0f;
    else if (rainTargetIntensity < 0.5f)
    {
        rainCooldown -= deltaTime;
        if (rainCooldown <= 0.0f)
        {
            rainTargetIntensity = 1.0f;
            rainTimer = RainDurationMin
                + static_cast<float>(std::rand() % static_cast<int>(RainDurationRange * 100)) / 100.0f;
        }
    }
    else
    {
        rainTimer -= deltaTime;
        if (rainTimer <= 0.0f)
        {
            rainTargetIntensity = 0.0f;
            rainCooldown = RainCooldownMin
                + static_cast<float>(std::rand() % static_cast<int>(RainCooldownRange * 100)) / 100.0f;
        }
    }

    // Move rainIntensity towards the target a small step each frame.
    float diff = rainTargetIntensity - rainIntensity;
    float step = RainFadeSpeed * deltaTime;
    rainIntensity += (std::abs(diff) < step) ? diff : (diff > 0.0f ? step : -step);

    audioSystem.setRainVolume(rainIntensity);
}

// Checks whether the score has crossed the next milestone and plays a fanfare if so.
// Uses a while loop so multiple thresholds crossed in one frame are all caught.
void MainGame::checkMilestones()
{
    if (isGameOver || nextMilestoneIndex >= NumMilestones) return;

    while (nextMilestoneIndex < NumMilestones && score >= MilestoneScores[nextMilestoneIndex])
    {
        audioSystem.playBeep(MilestoneBeepFreq1, MilestoneBeepDur,        MilestoneBeepVol);
        audioSystem.playBeep(MilestoneBeepFreq2, MilestoneBeepDur * 2.0f, MilestoneBeepVol);

        ScorePopup popup;
        popup.timeRemaining = 1.8f;
        popup.screenX       = gameDisplay.getWidth()  * 0.5f;
        popup.screenY       = gameDisplay.getHeight() * 0.40f;
        popup.text          = "SCORE: " + std::to_string(MilestoneScores[nextMilestoneIndex]) + "!";
        popup.color         = glm::vec4(1.0f, 0.85f, 0.0f, 1.0f);
        scorePopups.push_back(popup);

        nextMilestoneIndex++;
    }
}

// Called every frame once the game ends. Saves the high score and plays the jingle
// exactly once, then does nothing on every subsequent call.
void MainGame::handleGameOver()
{
    if (!isGameOver || hasHighScoreBeenSaved) return;

    hasHighScoreBeenSaved = true;
    gameOverMenuSelection = 0;
    audioSystem.pauseMusic();

    if (score > highScore)
    {
        highScore = score;
        saveHighScore();
    }

    totalSessions++;
    totalDistanceAllTime += distanceTraveled;
    saveStats();

    audioSystem.playGameOverJingle();
}
