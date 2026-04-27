#pragma once

// All the tuning values used across the game logic.
// Collected here so they are easy to find and adjust without digging through code.


// Train speed in world units per second. Increases with distance up to a cap.
static const float MinGameSpeed     = 5.0f;
static const float SpeedAccelFactor = 0.03f;
static const float MaxGameSpeed     = 15.0f;

// How many world units the train travels before one full day and night cycle completes.
static const float DayNightHalfPeriod = 600.0f;

// How much the train tilts sideways when changing lanes, in radians.
static const float LaneChangeLeanAngle = 0.28f;
// How quickly the lean returns to upright after a lane change.
static const float LeanSmoothingSpeed  = 9.0f;
// How quickly the train slides across to the target lane position.
static const float LaneSmoothingSpeed  = 10.0f;

// How fast the collectible boxes rotate, in radians per second.
static const float BoxSpinSpeed = 1.8f;

// How far ahead of the train new objects are placed when they spawn.
static const float SpawnLookaheadDistance = 25.0f;

// Minimum Z gap required between two objects in the same lane before a spawn is blocked.
static const float LaneBlockedRadius = 3.0f;

// Objects this far behind the train are removed from the world.
static const float ObjectCullDistance = 5.0f;

// How long between box spawns in seconds. The interval shrinks as the score goes up.
static const float BoxSpawnIntervalMin       = 1.5f;
static const float BoxSpawnIntervalRange     = 3.0f;   // random seconds added on top of the minimum
static const float BoxSpawnScoreRange        = 400.0f; // score at which the minimum scale is reached
static const float BoxSpawnMinScale          = 0.4f;   // minimum interval multiplier (40 percent of base)
static const float BoxSpawnDoublePointsScale = 0.55f;  // extra speed-up during a double points window

// How long between rock spawns in seconds.
static const float RockSpawnIntervalMin   = 2.0f;
static const float RockSpawnIntervalRange = 3.0f;

// Chance out of 100 that a second rock spawns next to the first one.
static const int CompanionRockChance = 20;

// How long between one-up heart spawns in seconds.
static const float OneUpSpawnIntervalMin   = 8.0f;
static const float OneUpSpawnIntervalRange = 7.0f;

// Minimum timer value kept when at full lives so a one-up cannot appear immediately.
static const float OneUpSpawnMinimumDelay = 2.0f;

// Half-size of the invisible collision box on both X and Z axes.
static const float CollisionRadius = 1.5f;

// How long the floating collect animation plays after picking something up.
static const float CollectAnimDuration = 0.4f;

// How long a score popup label stays on screen before fading out.
static const float ScorePopupDuration = 0.9f;

// How many boxes in a row the player must collect to activate each combo multiplier.
static const int ComboThresholdDouble = 3;  // 2x multiplier
static const int ComboThresholdTriple = 6;  // 3x multiplier

// Points earned per box variant (small, medium, large). Spawn weights: 60% / 30% / 10%.
static const int BoxScoreValues[3] = { 1, 3, 8 };

// Sound effect settings: frequency in Hz, duration in seconds, volume between 0 and 1.
static const float BoxCollectBeepFreq     = 880.0f;
static const float BoxCollectBeepDuration = 0.12f;
static const float BoxCollectBeepVolume   = 0.45f;

static const float RockHitBeepFreq     = 180.0f;
static const float RockHitBeepDuration = 0.25f;
static const float RockHitBeepVolume   = 0.7f;

static const float OneUpCollectBeepFreq     = 1200.0f;
static const float OneUpCollectBeepDuration = 0.14f;
static const float OneUpCollectBeepVolume   = 0.8f;

// How long the red screen flash and camera shake last after hitting a rock.
static const float HitFlashDuration    = 0.4f;
static const float CameraShakeDuration = 0.3f;

// Sound played once when the train first reaches the top speed.
static const float MaxSpeedBeepFreq     = 660.0f;
static const float MaxSpeedBeepDuration = 0.18f;
static const float MaxSpeedBeepVolume   = 0.5f;

// SDL_GetTicks returns milliseconds; divide by this to get seconds.
static const float MsToSeconds = 1000.0f;

// How long the double points window lasts and how long before the next one triggers.
static const float DoublePointsDuration      = 8.0f;
static const float DoublePointsCooldownMin   = 35.0f;
static const float DoublePointsCooldownRange = 25.0f;

// Sound played when a double points window starts.
static const float DoublePointsBeepFreq1 = 1047.0f;  // C6
static const float DoublePointsBeepFreq2 = 1319.0f;  // E6
static const float DoublePointsBeepDur   = 0.10f;
static const float DoublePointsBeepVol   = 0.55f;

// Score values that trigger a milestone fanfare.
static const int   MilestoneScores[] = { 50, 100, 250, 500, 1000, 2500, 5000 };
static const int   NumMilestones     = 7;

// Sound played when a milestone is reached.
static const float MilestoneBeepFreq1 = 523.0f;  // C5
static const float MilestoneBeepFreq2 = 659.0f;  // E5
static const float MilestoneBeepDur   = 0.10f;
static const float MilestoneBeepVol   = 0.50f;

// How long rain lasts and how long the gap between rain bursts is, in seconds.
static const float RainCooldownMin   = 50.0f;
static const float RainCooldownRange = 50.0f;
static const float RainDurationMin   = 15.0f;
static const float RainDurationRange = 20.0f;

// How fast the rain fades in and out (intensity units per second, so about 3 seconds to full rain).
static const float RainFadeSpeed = 0.35f;

// How long each object type waits before the very first spawn at game start.
static const float InitialBoxSpawnTimer   = 2.0f;
static const float InitialRockSpawnTimer  = 3.0f;
static const float InitialOneUpSpawnTimer = 10.0f;
