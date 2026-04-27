// Handles spawning new objects onto the track and checking for collisions with the train.
// Each object type (box, rock, one-up) has a pair of functions: one to spawn it
// and one to check whether the train has hit or collected it.

#include "MainGame.h"
#include "Constants.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <cstdlib>
#include <string>

// Counts down the box spawn timer and places a new collectible box in a random lane
// when it runs out. The spawn rate gets faster as the player's score increases.
void MainGame::trySpawnBox(float deltaTime)
{
    boxSpawnTimer -= deltaTime;
    if (boxSpawnTimer > 0.0f) return;

    BoxObject newBox;
    newBox.lane             = std::rand() % 3;
    newBox.worldZ           = distanceTraveled + SpawnLookaheadDistance;
    newBox.collected        = false;
    newBox.collectAnimTimer = 0.0f;

    // Weighted random variant: 60% small, 30% medium, 10% large.
    int variantRoll    = std::rand() % 10;
    newBox.meshVariant = (variantRoll < 6) ? 0 : (variantRoll < 9) ? 1 : 2;

    // Skip the spawn if a rock is already sitting in that lane at the same position.
    bool laneBlocked = false;
    for (auto& rock : activeRocks)
        if (!rock.collected && rock.lane == newBox.lane
            && std::abs(rock.worldZ - newBox.worldZ) < LaneBlockedRadius)
        { laneBlocked = true; break; }

    if (!laneBlocked)
        activeBoxes.push_back(newBox);

    // Scale the interval down as the score rises, with a floor so it never gets too fast.
    float spawnScale = glm::max(1.0f - (float)score / BoxSpawnScoreRange, BoxSpawnMinScale);
    if (doublePointsTimer > 0.0f) spawnScale *= BoxSpawnDoublePointsScale;
    float rawInterval = BoxSpawnIntervalMin
        + static_cast<float>(std::rand() % static_cast<int>(BoxSpawnIntervalRange * 100)) / 100.0f;
    boxSpawnTimer = rawInterval * spawnScale;
}

// Checks each uncollected box to see if the train is on top of it.
// On a hit: increments the combo, awards points, starts the collect animation,
// and shows a floating score label above where the box was.
// Boxes that fall behind the train or finish their animation are removed.
void MainGame::processBoxCollections()
{
    activeBoxes.erase(
        std::remove_if(activeBoxes.begin(), activeBoxes.end(), [&](BoxObject& box)
        {
            if (!box.collected
                && std::abs(LANE_POSITIONS[box.lane] - trainWorldX) < CollisionRadius
                && std::abs(box.worldZ - distanceTraveled) < CollisionRadius)
            {
                box.collected        = true;
                box.collectAnimTimer = CollectAnimDuration;
                combo++;

                // Raise pitch with each consecutive collect, up to a sensible limit.
                float pitchScale = 1.0f + glm::min(combo * 0.065f, 0.52f);
                audioSystem.playBeep(BoxCollectBeepFreq * pitchScale, BoxCollectBeepDuration, BoxCollectBeepVolume);

                int multiplier   = (combo >= ComboThresholdTriple) ? 3
                                 : (combo >= ComboThresholdDouble) ? 2 : 1;
                int pointsEarned = BoxScoreValues[box.meshVariant] * multiplier;
                if (doublePointsTimer > 0.0f) pointsEarned *= 2;
                score += pointsEarned;

                // Project the box position to screen space so the popup appears over it.
                glm::vec4 clipPos = renderer.getCamera().getViewProjection()
                    * glm::vec4(LANE_POSITIONS[box.lane], 0.4f, box.worldZ, 1.0f);

                if (clipPos.w > 0.0f)
                {
                    ScorePopup popup;
                    popup.timeRemaining = ScorePopupDuration;
                    popup.screenX       = (clipPos.x / clipPos.w * 0.5f + 0.5f) * gameDisplay.getWidth();
                    popup.screenY       = (1.0f - (clipPos.y / clipPos.w * 0.5f + 0.5f)) * gameDisplay.getHeight();
                    popup.text          = "+" + std::to_string(pointsEarned);
                    popup.color = multiplier >= 3 ? glm::vec4(1.0f, 0.85f, 0.0f, 1.0f)   // gold for 3x
                                : multiplier >= 2 ? glm::vec4(1.0f, 0.60f, 0.05f, 1.0f)  // orange for 2x
                                :                   glm::vec4(1.0f, 1.0f,  1.0f, 1.0f);  // white for 1x
                    scorePopups.push_back(popup);
                }
            }

            // Keep the box while its animation is playing, remove it once done or out of range.
            return box.collected
                ? (box.collectAnimTimer <= 0.0f)
                : (box.worldZ < distanceTraveled - ObjectCullDistance);
        }),
        activeBoxes.end()
    );
}

// Counts down the rock spawn timer and places a new obstacle on the track when it runs out.
// Sometimes a second companion rock is placed in the lane next to the first one.
void MainGame::trySpawnRock(float deltaTime)
{
    rockSpawnTimer -= deltaTime;
    if (rockSpawnTimer > 0.0f) return;

    BoxObject newRock;
    newRock.lane             = std::rand() % 3;
    newRock.worldZ           = distanceTraveled + SpawnLookaheadDistance;
    newRock.collected        = false;
    newRock.meshVariant      = 0;
    newRock.collectAnimTimer = 0.0f;

    // Block the spawn if a box, existing rock, or one-up is already nearby in that lane.
    bool laneBlocked = false;
    for (auto& box : activeBoxes)
        if (!box.collected && box.lane == newRock.lane
            && std::abs(box.worldZ - newRock.worldZ) < LaneBlockedRadius)
        { laneBlocked = true; break; }
    if (!laneBlocked)
        for (auto& existingRock : activeRocks)
            if (!existingRock.collected && existingRock.lane == newRock.lane
                && std::abs(existingRock.worldZ - newRock.worldZ) < LaneBlockedRadius)
            { laneBlocked = true; break; }
    if (!laneBlocked)
        for (auto& oneUp : activeOneUps)
            if (!oneUp.collected && oneUp.lane == newRock.lane
                && std::abs(oneUp.worldZ - newRock.worldZ) < LaneBlockedRadius)
            { laneBlocked = true; break; }

    if (!laneBlocked)
    {
        activeRocks.push_back(newRock);

        // Occasionally add a second rock in the adjacent lane at the same Z position.
        if (std::rand() % 100 < CompanionRockChance)
        {
            // If the first rock is in the centre, pick left or right randomly; otherwise use the centre.
            int companionLane = (newRock.lane == 1) ? ((std::rand() % 2 == 0) ? 0 : 2) : 1;

            BoxObject companionRock;
            companionRock.lane             = companionLane;
            companionRock.worldZ           = newRock.worldZ;
            companionRock.collected        = false;
            companionRock.meshVariant      = 0;
            companionRock.collectAnimTimer = 0.0f;

            bool companionBlocked = false;
            for (auto& box : activeBoxes)
                if (!box.collected && box.lane == companionLane
                    && std::abs(box.worldZ - companionRock.worldZ) < LaneBlockedRadius)
                { companionBlocked = true; break; }
            if (!companionBlocked)
                for (auto& existingRock : activeRocks)
                    if (!existingRock.collected && existingRock.lane == companionLane
                        && std::abs(existingRock.worldZ - companionRock.worldZ) < LaneBlockedRadius)
                    { companionBlocked = true; break; }
            if (!companionBlocked)
                for (auto& oneUp : activeOneUps)
                    if (!oneUp.collected && oneUp.lane == companionLane
                        && std::abs(oneUp.worldZ - companionRock.worldZ) < LaneBlockedRadius)
                    { companionBlocked = true; break; }

            if (!companionBlocked)
                activeRocks.push_back(companionRock);
        }
    }

    rockSpawnTimer = RockSpawnIntervalMin
        + static_cast<float>(std::rand() % static_cast<int>(RockSpawnIntervalRange * 100)) / 100.0f;
}

// Checks each uncollected rock to see if the train has driven into it.
// On a hit: resets the combo (with a popup if a multiplier was active),
// takes away a life, and triggers the red flash and camera shake feedback.
// Rocks that fall behind the train are also removed.
void MainGame::processRockCollisions()
{
    activeRocks.erase(
        std::remove_if(activeRocks.begin(), activeRocks.end(), [&](BoxObject& rock)
        {
            if (!rock.collected
                && std::abs(LANE_POSITIONS[rock.lane] - trainWorldX) < CollisionRadius
                && std::abs(rock.worldZ - distanceTraveled) < CollisionRadius)
            {
                rock.collected = true;

                // Only show the combo lost popup if the player actually had a multiplier.
                if (combo >= ComboThresholdDouble)
                {
                    glm::vec4 clipPos = renderer.getCamera().getViewProjection()
                        * glm::vec4(trainWorldX, 1.5f, distanceTraveled, 1.0f);
                    if (clipPos.w > 0.0f)
                    {
                        ScorePopup popup;
                        popup.timeRemaining = ScorePopupDuration;
                        popup.screenX = (clipPos.x / clipPos.w * 0.5f + 0.5f) * gameDisplay.getWidth();
                        popup.screenY = (1.0f - (clipPos.y / clipPos.w * 0.5f + 0.5f)) * gameDisplay.getHeight();
                        popup.text  = "Combo lost :(";
                        popup.color = glm::vec4(1.0f, 0.2f, 0.2f, 1.0f);
                        scorePopups.push_back(popup);
                    }
                }

                combo = 0;
                lives--;
                hitFlashTimer    = HitFlashDuration;
                cameraShakeTimer = CameraShakeDuration;
                audioSystem.playBeep(RockHitBeepFreq, RockHitBeepDuration, RockHitBeepVolume);

                if (lives <= 0)
                    isGameOver = true;
            }

            return rock.worldZ < distanceTraveled - ObjectCullDistance;
        }),
        activeRocks.end()
    );
}

// Counts down the one-up spawn timer and places a heart pickup on the track.
// One-ups only appear when the player is missing at least one life.
// When at full health, the timer is held at a minimum so one cannot appear straight
// away after the player next loses a life.
void MainGame::trySpawnOneUp(float deltaTime)
{
    if (isGameOver) return;

    if (lives < maxLives)
    {
        oneUpSpawnTimer -= deltaTime;
        if (oneUpSpawnTimer > 0.0f) return;

        OneUpObject newOneUp;
        newOneUp.lane             = std::rand() % 3;
        newOneUp.worldZ           = distanceTraveled + SpawnLookaheadDistance;
        newOneUp.collected        = false;
        newOneUp.collectAnimTimer = 0.0f;

        bool laneBlocked = false;
        for (auto& box : activeBoxes)
            if (!box.collected && box.lane == newOneUp.lane
                && std::abs(box.worldZ - newOneUp.worldZ) < LaneBlockedRadius)
            { laneBlocked = true; break; }
        if (!laneBlocked)
            for (auto& rock : activeRocks)
                if (!rock.collected && rock.lane == newOneUp.lane
                    && std::abs(rock.worldZ - newOneUp.worldZ) < LaneBlockedRadius)
                { laneBlocked = true; break; }
        if (!laneBlocked)
            for (auto& existingOneUp : activeOneUps)
                if (!existingOneUp.collected && existingOneUp.lane == newOneUp.lane
                    && std::abs(existingOneUp.worldZ - newOneUp.worldZ) < LaneBlockedRadius)
                { laneBlocked = true; break; }

        if (!laneBlocked)
            activeOneUps.push_back(newOneUp);

        oneUpSpawnTimer = OneUpSpawnIntervalMin
            + static_cast<float>(std::rand() % static_cast<int>(OneUpSpawnIntervalRange * 100)) / 100.0f;
    }
    else
    {
        // Keep a floor on the timer so the one-up cannot appear immediately after the next life loss.
        oneUpSpawnTimer = glm::max(oneUpSpawnTimer, OneUpSpawnMinimumDelay);
    }
}

// Checks each uncollected one-up to see if the train has driven over it.
// On a hit: restores one life, starts the collect animation, and shows a popup.
// One-ups that fall behind the train are also removed.
void MainGame::processOneUpCollections()
{
    activeOneUps.erase(
        std::remove_if(activeOneUps.begin(), activeOneUps.end(), [&](OneUpObject& oneUp)
        {
            if (!oneUp.collected
                && std::abs(LANE_POSITIONS[oneUp.lane] - trainWorldX) < CollisionRadius
                && std::abs(oneUp.worldZ - distanceTraveled) < CollisionRadius)
            {
                oneUp.collected        = true;
                oneUp.collectAnimTimer = CollectAnimDuration;
                audioSystem.playBeep(OneUpCollectBeepFreq, OneUpCollectBeepDuration, OneUpCollectBeepVolume);
                lives = std::min(lives + 1, maxLives);

                glm::vec4 clipPos = renderer.getCamera().getViewProjection()
                    * glm::vec4(LANE_POSITIONS[oneUp.lane], 0.6f, oneUp.worldZ, 1.0f);

                ScorePopup popup;
                popup.timeRemaining = ScorePopupDuration;
                popup.text          = "+1UP";
                popup.color         = glm::vec4(0.4f, 1.0f, 0.4f, 1.0f);

                if (clipPos.w > 0.0f)
                {
                    popup.screenX = (clipPos.x / clipPos.w * 0.5f + 0.5f) * gameDisplay.getWidth();
                    popup.screenY = (1.0f - (clipPos.y / clipPos.w * 0.5f + 0.5f)) * gameDisplay.getHeight();
                }
                else
                {
                    // Fallback: if the pickup is behind the camera, centre the popup on screen.
                    popup.screenX = gameDisplay.getWidth()  * 0.5f;
                    popup.screenY = gameDisplay.getHeight() * 0.5f;
                }

                scorePopups.push_back(popup);
            }

            return oneUp.collected
                ? (oneUp.collectAnimTimer <= 0.0f)
                : (oneUp.worldZ < distanceTraveled - ObjectCullDistance);
        }),
        activeOneUps.end()
    );
}
