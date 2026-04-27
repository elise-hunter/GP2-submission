// Core game class. Sets everything up, runs the main loop,
// handles player input, and calls the update and render steps each frame.

#include "MainGame.h"
#include "Constants.h"
#include <glm/glm.hpp>
#include <cstdlib>
#include <ctime>

// Sets up all member variables and loads the saved high score and stats.
MainGame::MainGame()
{
    gameState                = GameState::MENU;
    lastFrameTime            = 0;
    menuTime                 = 0.0f;
    pauseMenuSelection       = 0;
    gameOverMenuSelection    = 0;
    doublePointsTimer        = 0.0f;
    rainIntensity            = 0.0f;
    rainTargetIntensity      = 0.0f;
    rainTimer                = 0.0f;
    rainCooldown             = 0.0f;
    doublePointsCooldown     = 0.0f;
    nextMilestoneIndex       = 0;
    totalSessions            = 0;
    totalDistanceAllTime     = 0.0f;
    loadStats();
    hitFlashTimer            = 0.0f;
    cameraShakeTimer         = 0.0f;
    isGameOver               = false;
    highScore                = 0;
    hasHighScoreBeenSaved    = false;
    hasShownMaxSpeedWarning  = false;
    loadHighScore();
    std::srand((unsigned)std::time(nullptr));
    resetGame();
}

MainGame::~MainGame() {}

// Sets all game values back to the starting state ready for a new round.
void MainGame::resetGame()
{
    distanceTraveled        = 0.0f;
    boxSpinAngle            = 0.0f;
    trainLeanAngle          = 0.0f;
    currentLane             = 1;
    trainWorldX             = LANE_POSITIONS[1];
    boxSpawnTimer           = InitialBoxSpawnTimer;
    rockSpawnTimer          = InitialRockSpawnTimer;
    oneUpSpawnTimer         = InitialOneUpSpawnTimer;
    score                   = 0;
    lives                   = 3;
    combo                   = 0;
    maxLives                = 3;
    isGameOver              = false;
    gameOverMenuSelection   = 0;
    hasHighScoreBeenSaved   = false;
    hasShownMaxSpeedWarning = false;
    doublePointsTimer       = 0.0f;
    rainIntensity           = 0.0f;
    rainTargetIntensity     = 0.0f;
    rainTimer               = 0.0f;
    rainCooldown            = RainCooldownMin
        + static_cast<float>(std::rand() % static_cast<int>(RainCooldownRange * 100)) / 100.0f;
    audioSystem.setRainVolume(0.0f);
    doublePointsCooldown    = DoublePointsCooldownMin
        + static_cast<float>(std::rand() % static_cast<int>(DoublePointsCooldownRange * 100)) / 100.0f;
    nextMilestoneIndex      = 0;
    activeBoxes.clear();
    activeRocks.clear();
    activeOneUps.clear();
    scorePopups.clear();
    audioSystem.clearQueue();
    cameraShakeTimer = 0.0f;
}

// Entry point: sets up all subsystems then starts the game loop.
void MainGame::run()
{
    initSystems();
    gameLoop();
}

// Creates the window, initialises audio, and sets up the renderer.
void MainGame::initSystems()
{
    gameDisplay.initDisplay();
    audioSystem.init();
    audioSystem.playMusic();
    audioSystem.setMusicVolume(96);
    renderer.init(gameDisplay);
    lastFrameTime = SDL_GetTicks();
}

// Runs every frame until the player quits: polls input, steps logic, then renders.
void MainGame::gameLoop()
{
    while (gameState != GameState::EXIT)
    {
        processInput();

        unsigned int currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastFrameTime) / MsToSeconds;
        lastFrameTime = currentTime;

        if (gameState == GameState::MENU)
        {
            menuTime += deltaTime;
            renderer.renderMenuFrame(gameDisplay, menuTime);
            continue;
        }

        if (gameState != GameState::PAUSED)
            updateGame(deltaTime);

        // Fill in the render state struct with all current game data for this frame.
        GameRenderState renderState;
        renderState.deltaTime        = deltaTime;
        // Triangle wave so the day and night cycle keeps looping back and forth.
        float dnCycle = fmod(distanceTraveled / DayNightHalfPeriod, 2.0f);
        renderState.dayNightProgress   = (dnCycle < 1.0f) ? dnCycle : 2.0f - dnCycle;
        renderState.hitFlashTimer      = hitFlashTimer;
        renderState.trainLeanAngle     = trainLeanAngle;
        renderState.distanceTraveled   = distanceTraveled;
        renderState.boxSpinAngle       = boxSpinAngle;
        renderState.cameraShakeTimer   = cameraShakeTimer;
        renderState.gameSpeed          = calculateGameSpeed();
        renderState.trainPosition      = glm::vec3(trainWorldX, 0.0f, distanceTraveled);
        renderState.isGameOver            = isGameOver;
        renderState.gameOverMenuSelection = gameOverMenuSelection;
        renderState.isPaused              = (gameState == GameState::PAUSED);
        renderState.pauseMenuSelection = pauseMenuSelection;
        renderState.doublePointsTimer  = doublePointsTimer;
        renderState.rainIntensity      = rainIntensity;
        renderState.totalSessions        = totalSessions;
        renderState.totalDistanceAllTime = totalDistanceAllTime;
        renderState.score              = score;
        renderState.lives              = lives;
        renderState.maxLives           = maxLives;
        renderState.combo              = combo;
        renderState.highScore          = highScore;
        renderState.activeBoxes        = &activeBoxes;
        renderState.activeRocks        = &activeRocks;
        renderState.activeOneUps       = &activeOneUps;
        renderState.scorePopups        = &scorePopups;

        renderer.renderFrame(gameDisplay, renderState);
    }
}

// Polls SDL events and maps keyboard input to game actions.
void MainGame::processInput()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_QUIT:
            gameState = GameState::EXIT;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym)
            {
            case SDLK_ESCAPE:
                if (gameState == GameState::PLAY && !isGameOver)
                {
                    gameState = GameState::PAUSED;
                    pauseMenuSelection = 0;
                    audioSystem.pauseMusic();
                }
                else if (gameState == GameState::PAUSED)
                {
                    gameState = GameState::PLAY;
                    audioSystem.resumeMusic();
                }
                else
                    gameState = GameState::EXIT;
                break;

            case SDLK_RETURN:
            case SDLK_KP_ENTER:
                if (gameState == GameState::MENU)
                {
                    resetGame();
                    gameState = GameState::PLAY;
                }
                else if (gameState == GameState::PAUSED)
                {
                    if (pauseMenuSelection == 0)
                    {
                        audioSystem.resumeMusic();
                        gameState = GameState::PLAY;
                    }
                    else if (pauseMenuSelection == 1)
                    {
                        audioSystem.resumeMusic();
                        resetGame();
                        gameState = GameState::MENU;
                    }
                    else
                        gameState = GameState::EXIT;
                }
                else if (isGameOver)
                {
                    if (gameOverMenuSelection == 0)
                    {
                        // Restart from scratch: stop and replay music from the beginning.
                        audioSystem.stopMusic();
                        audioSystem.playMusic();
                        resetGame();
                    }
                    else if (gameOverMenuSelection == 1)
                    {
                        audioSystem.resumeMusic();
                        resetGame();
                        gameState = GameState::MENU;
                    }
                    else
                        gameState = GameState::EXIT;
                }
                break;

            // Debug camera controls
            case SDLK_w: if (!isGameOver) renderer.getCamera().MoveForward(0.5f); break;
            case SDLK_s: if (!isGameOver) renderer.getCamera().MoveBack(0.5f);    break;
            case SDLK_a: if (!isGameOver) renderer.getCamera().MoveLeft(0.5f);    break;
            case SDLK_d: if (!isGameOver) renderer.getCamera().MoveRight(0.5f);   break;
            case SDLK_q: if (!isGameOver) renderer.getCamera().MoveUp(0.5f);      break;
            case SDLK_e: if (!isGameOver) renderer.getCamera().MoveDown(0.5f);    break;

            case SDLK_UP:
                if (gameState == GameState::PAUSED)
                    pauseMenuSelection = (pauseMenuSelection + 2) % 3;
                else if (isGameOver)
                    gameOverMenuSelection = (gameOverMenuSelection + 2) % 3;
                else if (currentLane < 2)
                {
                    currentLane++;
                    trainLeanAngle = -LaneChangeLeanAngle;
                }
                break;

            case SDLK_DOWN:
                if (gameState == GameState::PAUSED)
                    pauseMenuSelection = (pauseMenuSelection + 1) % 3;
                else if (isGameOver)
                    gameOverMenuSelection = (gameOverMenuSelection + 1) % 3;
                else if (currentLane > 0)
                {
                    currentLane--;
                    trainLeanAngle = LaneChangeLeanAngle;
                }
                break;
            }
            break;
        }
    }
}

// Runs each simulation step in order for one frame.
void MainGame::updateGame(float deltaTime)
{
    updateTimers(deltaTime);
    updateCollectAnimTimers(deltaTime);

    float gameSpeed = calculateGameSpeed();

    if (!isGameOver)
    {
        distanceTraveled += gameSpeed * deltaTime;
        boxSpinAngle     += BoxSpinSpeed * deltaTime;

        // Show a one-time popup and play a sound when the train first hits the speed cap.
        if (!hasShownMaxSpeedWarning && gameSpeed >= MaxGameSpeed)
        {
            hasShownMaxSpeedWarning = true;
            audioSystem.playBeep(MaxSpeedBeepFreq, MaxSpeedBeepDuration, MaxSpeedBeepVolume);

            ScorePopup popup;
            popup.timeRemaining = ScorePopupDuration * 1.5f;
            popup.screenX       = gameDisplay.getWidth()  * 0.5f;
            popup.screenY       = gameDisplay.getHeight() * 0.3f;
            popup.text          = "MAX SPEED!";
            popup.color         = glm::vec4(1.0f, 0.45f, 0.0f, 1.0f);
            scorePopups.push_back(popup);
        }
    }

    // Smoothly slide the train towards its target lane position.
    trainWorldX += (LANE_POSITIONS[currentLane] - trainWorldX)
        * glm::min(LaneSmoothingSpeed * deltaTime, 1.0f);

    trySpawnBox(deltaTime);
    processBoxCollections();
    trySpawnRock(deltaTime);
    processRockCollisions();
    trySpawnOneUp(deltaTime);
    processOneUpCollections();
    updateDoublePoints(deltaTime);
    updateRain(deltaTime);
    checkMilestones();
    handleGameOver();
}
