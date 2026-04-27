// Loads all rendering resources and draws the game scene each frame.

#include "Renderer.h"
#include <SDL/SDL.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <string>
#include <algorithm>
#include <cmath>

// Camera position expressed as an offset from the train in world space.
// The track runs along the Z axis, so X moves the camera left/right of the track.
static const float CameraFollowX    = -14.0f;
static const float CameraFollowY    =   7.5f;
static const float CameraLookYOffset =   2.5f;  // look target is slightly above the train base

// Camera shake parameters applied when the player hits a rock
static const float CameraShakeStrengthMult = 0.5f;   // amplitude = cameraShakeTimer * this value
static const float CameraShakeFreqX        = 53.7f;  // horizontal shake speed (chosen so X and Y never sync into a pattern)
static const float CameraShakeFreqY        = 41.3f;  // vertical shake speed
static const float CameraShakeYScale       = 0.5f;   // vertical shake is half the amplitude of horizontal

// Background clear colour interpolated between day and night
static const float ClearDayR   = 0.45f;
static const float ClearDayG   = 0.60f;
static const float ClearDayB   = 0.30f;
static const float ClearNightR = 0.05f;
static const float ClearNightG = 0.07f;
static const float ClearNightB = 0.03f;

// Track tile rendering
static const float        TileDrawRange = 30.0f;   // tiles are drawn this far in front and behind the train
static const float        TileLength   =  2.0f;   // one track tile spans this many world units along Z
static const float        TileYOffset  = -0.15f;  // tiles sit slightly below Y=0 to appear under the train

// Lane warning flash shown on tiles when a rock is approaching
static const unsigned int WarningFlashIntervalMs = 250;    // flash toggles every this many milliseconds
static const float        WarningFlashBrightness = 0.75f;
static const float        RockWarningMinDistance = 1.5f;   // do not flash if the rock is already this close
static const float        RockWarningMaxDistance = 25.0f;  // maximum look-ahead for the warning

// Rock rendering
static const float RockYPosition = 0.3f;
static const float RockScale     = 2.5f;

// Heart (one-up) rendering
static const float HeartYPosition = 0.6f;
static const float HeartScale     = 4.0f;

// Train rendering
static const float TrainScale = 2.0f;

// Collect animation (values must match CollectAnimDuration in MainGame.cpp)
static const float CollectAnimDuration = 0.4f;   // seconds the animation plays
static const float CollectFloatHeight  = 3.5f;   // world units the object rises during the animation
static const float CollectScaleBoost   = 0.4f;   // maximum extra scale multiplier at animation end
static const float HitFlashDuration    = 0.4f;   // seconds the red hit flash lasts; used to normalise to [0,1]

// Fence rendering
static const float FenceScale     = 5.0f;
static const float FenceXPosition = 6.0f;
static const float FenceDrawRange = 40.0f;

// Bush rendering (distances relative to the train along Z)
static const float BushDrawBackRange    = 12.0f;
static const float BushDrawForwardRange = 38.0f;

// Tree rendering
static const float TreeDrawBackRange    = 12.0f;
static const float TreeDrawForwardRange = 38.0f;

// tree-pine2.obj model height is 0.83 units; tree-pine.obj is 2.0 units.
// Scale pineTreeMeshB up by this ratio so both models appear the same visual height.
static const float PineBModelScaleRatio = 2.403f;

// HUD element positions in screen pixels
static const float HudLeftMargin = 20.0f;
static const float HudScoreLineY = 40.0f;
static const float HudLivesLineY = 85.0f;
static const float HudComboLineY = 130.0f;

// Score popup animation
static const float PopupDriftSpeed = 55.0f;  // pixels per second the popup drifts upward
static const float PopupScaleRange = 0.4f;   // extra scale added as the popup fades out

// Game over / pause screen
static const float GameOverOverlayAlpha = 0.55f;
static const float GameOverPanelOffsetX = 190.0f;
static const float PauseOverlayAlpha    = 0.65f;

// HUD layout
static const float HudDoubleLineY = 175.0f;  // Y position for the 2X POINTS indicator

// Hemisphere lighting colours mixed between day and night using dayNightProgress
static const glm::vec3 DaySkyLightColor     (1.00f, 0.97f, 0.88f);
static const glm::vec3 NightSkyLightColor   (0.05f, 0.07f, 0.15f);
static const glm::vec3 DayGroundLightColor  (0.22f, 0.20f, 0.12f);
static const glm::vec3 NightGroundLightColor(0.02f, 0.03f, 0.05f);
static const glm::vec3 DayFogColor          (0.72f, 0.84f, 0.94f);
static const glm::vec3 NightFogColor        (0.01f, 0.02f, 0.08f);

GameRenderer::GameRenderer() {}

// Loads all meshes, textures, and shaders, and sets up the initial camera.
void GameRenderer::init(Display& display)
{
    // Load all 3D models from the resource folder
    trainMesh.loadModel("..\\res\\train.obj");
    trackTileMesh.loadModel("..\\res\\track.obj");

    // All three box size variants use the same mesh; scale differs at draw time
    boxMeshVariants[0].loadModel("..\\res\\box.obj");
    boxMeshVariants[1].loadModel("..\\res\\box.obj");
    boxMeshVariants[2].loadModel("..\\res\\box.obj");

    rockMesh.loadModel("..\\res\\rocks.obj");
    fenceMesh.loadModel("..\\res\\fence.obj");
    heartMesh.loadModel("..\\res\\heart.obj");
    pineTreeMesh.loadModel("..\\res\\tree-pine.obj");
    pineTreeMeshB.loadModel("..\\res\\tree-pine2.obj");

    // Sky is a full-screen quad drawn at the very back so everything else appears in front.
    // Bottom edge at y=-0.4 leaves the ground visible.
    Vertex skyVertices[] = {
        Vertex(glm::vec3(-1.0f, -0.4f, 0.9999f), glm::vec2(0.0f, 1.0f)),
        Vertex(glm::vec3( 1.0f, -0.4f, 0.9999f), glm::vec2(1.0f, 1.0f)),
        Vertex(glm::vec3( 1.0f,  1.0f, 0.9999f), glm::vec2(1.0f, 0.0f)),
        Vertex(glm::vec3(-1.0f,  1.0f, 0.9999f), glm::vec2(0.0f, 0.0f)),
    };
    unsigned int skyIndices[] = { 0, 1, 2, 0, 2, 3 };
    skyQuadMesh.init(skyVertices, 4, skyIndices, 6);

    // Full-screen quad used to darken the screen for the game over and pause overlays.
    Vertex overlayVertices[] = {
        Vertex(glm::vec3(-1.0f, -1.0f, 0.9999f), glm::vec2(0.0f, 0.0f)),
        Vertex(glm::vec3( 1.0f, -1.0f, 0.9999f), glm::vec2(1.0f, 0.0f)),
        Vertex(glm::vec3( 1.0f,  1.0f, 0.9999f), glm::vec2(1.0f, 1.0f)),
        Vertex(glm::vec3(-1.0f,  1.0f, 0.9999f), glm::vec2(0.0f, 1.0f)),
    };
    unsigned int overlayIndices[] = { 0, 1, 2, 0, 2, 3 };
    overlayQuadMesh.init(overlayVertices, 4, overlayIndices, 6);

    // All ground quads use this index order
    unsigned int groundIndices[] = { 0, 2, 1, 0, 3, 2 };
    glm::vec3    upNormal(0.0f, 1.0f, 0.0f);

    // Far-side grass plane extending beyond the mud strip to the horizon
    Vertex grassVertices[] = {
        Vertex(glm::vec3(-7.0f,  0.0f, -200.0f), glm::vec2(  0.0f,   0.0f), upNormal),
        Vertex(glm::vec3(200.0f, 0.0f, -200.0f), glm::vec2(103.5f,   0.0f), upNormal),
        Vertex(glm::vec3(200.0f, 0.0f,  400.0f), glm::vec2(103.5f, 270.0f), upNormal),
        Vertex(glm::vec3(-7.0f,  0.0f,  400.0f), glm::vec2(  0.0f, 270.0f), upNormal),
    };
    grassMesh.init(grassVertices, 4, groundIndices, 6);

    // Near-side strip between the camera and the mud; uses the grass texture
    Vertex nearsideVertices[] = {
        Vertex(glm::vec3(-30.0f, 0.0f, -200.0f), glm::vec2( 0.0f,   0.0f), upNormal),
        Vertex(glm::vec3( -7.0f, 0.0f, -200.0f), glm::vec2(11.5f,   0.0f), upNormal),
        Vertex(glm::vec3( -7.0f, 0.0f,  400.0f), glm::vec2(11.5f, 270.0f), upNormal),
        Vertex(glm::vec3(-30.0f, 0.0f,  400.0f), glm::vec2( 0.0f, 270.0f), upNormal),
    };
    nearsideGrassMesh.init(nearsideVertices, 4, groundIndices, 6);

    // Mud strip covering the track bed, drawn on top of the grass
    Vertex mudVertices[] = {
        Vertex(glm::vec3( -7.0f, 0.0f, -200.0f), glm::vec2( 0.0f,   0.0f), upNormal),
        Vertex(glm::vec3( 10.0f, 0.0f, -200.0f), glm::vec2( 7.7f,   0.0f), upNormal),
        Vertex(glm::vec3( 10.0f, 0.0f,  400.0f), glm::vec2( 7.7f, 270.0f), upNormal),
        Vertex(glm::vec3( -7.0f, 0.0f,  400.0f), glm::vec2( 0.0f, 270.0f), upNormal),
    };
    mudStripMesh.init(mudVertices, 4, groundIndices, 6);

    grassTexture.init("..\\res\\grass.png");
    mudTexture.init("..\\res\\dirt.png");
    groundNormalMap.init("..\\res\\ground_normal.png");

    // Bush billboard: a flat vertical quad that is rotated to face the camera each frame.
    // Bottom edge at Y=0 and top edge at Y=1 so it stands on the ground plane.
    Vertex bushVertices[] = {
        Vertex(glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        Vertex(glm::vec3( 1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        Vertex(glm::vec3( 1.0f, 1.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        Vertex(glm::vec3(-1.0f, 1.0f, 0.0f), glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
    };
    unsigned int bushIndices[] = { 0, 1, 2, 0, 2, 3 };
    bushBillboardMesh.init(bushVertices, 4, bushIndices, 6);

    bushTextures[0].init("..\\res\\bush1.png");
    bushTextures[1].init("..\\res\\bushAlt1.png");

    mainShader.init("..\\res\\shader.vert", "..\\res\\shader.frag");
    skyShader.init("..\\res\\sky.vert", "..\\res\\sky.frag");

    textRenderer.init("C:\\Windows\\Fonts\\comic.ttf", 48.0f,
        display.getWidth(), display.getHeight());

    camera.initCamera(
        glm::vec3(0.0f, 2.0f, -5.0f),
        glm::radians(65.0f),
        display.getWidth() / display.getHeight(),
        0.01f,
        1000.0f
    );
}

// Positions the camera, clears the screen, then calls each rendering sub-pass in order.
void GameRenderer::renderFrame(Display& display, const GameRenderState& state)
{
    // Move the camera to follow just behind and above the train
    camera.setPos(state.trainPosition + glm::vec3(CameraFollowX, CameraFollowY, 0.0f));
    camera.setLook(state.trainPosition + glm::vec3(0.0f, CameraLookYOffset, 0.0f));

    // Jiggle the camera using a sine wave when the player just hit a rock
    if (state.cameraShakeTimer > 0.0f)
    {
        float time          = SDL_GetTicks() * 0.001f;
        float shakeStrength = state.cameraShakeTimer * CameraShakeStrengthMult;
        camera.setPos(camera.getPos() + glm::vec3(
            sinf(time * CameraShakeFreqX) * shakeStrength,
            sinf(time * CameraShakeFreqY) * shakeStrength * CameraShakeYScale,
            0.0f
        ));
    }

    // Clear to a green that darkens toward dark green as night progresses
    glm::vec3 clearColor = glm::mix(
        glm::vec3(ClearDayR,   ClearDayG,   ClearDayB),
        glm::vec3(ClearNightR, ClearNightG, ClearNightB),
        state.dayNightProgress
    );
    display.clearDisplay(clearColor.r, clearColor.g, clearColor.b, 1.0f);

    float aspectRatio = display.getWidth() / display.getHeight();
    renderSky(state.dayNightProgress, aspectRatio);

    // Activate the main shader and set up the lighting values used by everything in the scene
    mainShader.Bind();
    mainShader.setFloat("hitFlash", 0.0f);
    mainShader.setVec3("colorMult", glm::vec3(1.0f, 1.0f, 1.0f));
    mainShader.setInt("diffuse",      0);
    mainShader.setInt("normalMap",    1);
    mainShader.setInt("hasNormalMap", 0);
    setLightingUniforms(state.dayNightProgress, state.trainPosition);
    mainShader.setFloat("rainIntensity", state.rainIntensity);

    renderGround(state.distanceTraveled);
    renderFence(state.distanceTraveled);
    renderTrackTiles(state.distanceTraveled, *state.activeRocks);
    renderBoxes(*state.activeBoxes, state.boxSpinAngle);
    renderOneUps(*state.activeOneUps, state.boxSpinAngle);
    renderRocks(*state.activeRocks);
    renderBushes(state.distanceTraveled);
    renderTrees(state.distanceTraveled);

    // Convert the hit flash timer into a 0-1 value the shader can use
    renderTrain(state.trainPosition, state.trainLeanAngle, state.hitFlashTimer / HitFlashDuration);

    renderRain(state.rainIntensity, SDL_GetTicks() * 0.001f, aspectRatio);
    renderHUD(display, state);

    display.setTitle("Score: " + std::to_string(state.score)
        + "   Lives: " + std::to_string(state.lives)
        + "   Speed: " + std::to_string((int)state.gameSpeed));

    display.swapBuffer();
}

// Sends the lighting and fog settings to the main shader.
// Call once per frame before drawing any world geometry.
void GameRenderer::setLightingUniforms(float dayNightProgress, glm::vec3 trainPosition)
{
    mainShader.setVec3("skyLight",    glm::mix(DaySkyLightColor,      NightSkyLightColor,    dayNightProgress));
    mainShader.setVec3("groundLight", glm::mix(DayGroundLightColor,   NightGroundLightColor, dayNightProgress));
    mainShader.setVec3("fogColor",    glm::mix(DayFogColor,           NightFogColor,         dayNightProgress));
    mainShader.setFloat("dayPhase",   dayNightProgress);
    mainShader.setVec3("trainPos",    trainPosition);
    mainShader.setVec3("cameraPos",   camera.getPos());
}

// Draws the sky gradient quad behind all geometry using the sky shader.
void GameRenderer::renderSky(float dayNightProgress, float aspectRatio)
{
    skyShader.Bind();
    skyShader.setFloat("dayPhase",        dayNightProgress);
    skyShader.setFloat("aspectRatio",     aspectRatio);
    skyShader.setFloat("overlayAlpha",    0.0f);
    skyShader.setFloat("rainMode",        0.0f);

    // Don't write to the depth buffer so the sky never blocks anything drawn after it
    glDepthMask(GL_FALSE);
    skyQuadMesh.draw();
    glDepthMask(GL_TRUE);
}

// Draws the ground layers: far grass, nearside grass, and mud strip.
// Polygon offsets push the layers apart slightly so they don't flicker where they overlap.
void GameRenderer::renderGround(float distanceTraveled)
{
    mainShader.setVec3("colorMult", glm::vec3(1.0f, 1.0f, 1.0f));

    // Slide the ground with the train so it's always underneath the camera
    objectTransform.SetPos(glm::vec3(0.0f, 0.0f, distanceTraveled));
    objectTransform.SetRot(glm::vec3(0.0f));
    objectTransform.SetScale(glm::vec3(1.0f));
    mainShader.Update(objectTransform, camera);

    // Shift the texture coordinates so the ground appears to scroll as the train moves
    mainShader.setVec2("uvOffset", glm::vec2(0.0f, distanceTraveled / 6.0f));

    // Mud renders above the grass, and track tiles render above both.
    mainShader.setInt("hasNormalMap", 1);
    groundNormalMap.Bind(1);

    glEnable(GL_POLYGON_OFFSET_FILL);

    glPolygonOffset(2.0f, 2.0f);
    grassTexture.Bind(0);
    grassMesh.draw();

    nearsideGrassMesh.draw();

    glPolygonOffset(1.0f, 1.0f);
    mudTexture.Bind(0);
    mudStripMesh.draw();

    glDisable(GL_POLYGON_OFFSET_FILL);
    mainShader.setInt("hasNormalMap", 0);
    mainShader.setVec3("colorMult", glm::vec3(1.0f, 1.0f, 1.0f));
}

// Draws track tiles for all three lanes across the visible range.
// Tiles in a lane flash when an uncollected rock is approaching in that lane.
void GameRenderer::renderTrackTiles(float distanceTraveled, const std::vector<BoxObject>& activeRocks)
{
    mainShader.setFloat("hitFlash", 0.0f);
    mainShader.setVec3("colorMult", glm::vec3(1.0f, 1.0f, 1.0f));
    mainShader.setVec2("uvOffset", glm::vec2(0.0f, 0.0f));

    if (trackTileMesh.hasTexture()) trackTileMesh.bindTexture(0);

    // Determine the range of tile indices visible around the train
    int firstTileIndex = (int)((distanceTraveled - TileDrawRange) / TileLength);
    int lastTileIndex  = (int)((distanceTraveled + TileDrawRange) / TileLength);

    // Flash state alternates on a fixed millisecond interval to create a blinking effect
    float warningBrightness = ((SDL_GetTicks() / WarningFlashIntervalMs) % 2 == 0) ? WarningFlashBrightness : 0.0f;

    for (int lane = 0; lane < 3; lane++)
    {
        for (int tileIndex = firstTileIndex; tileIndex <= lastTileIndex; tileIndex++)
        {
            float tileZ = (float)tileIndex * TileLength;

            // Check whether an uncollected rock in this lane is inside the warning range
            float flashAmount = 0.0f;
            for (auto& rock : activeRocks)
            {
                if (!rock.collected && rock.lane == lane
                    && rock.worldZ - distanceTraveled > RockWarningMinDistance
                    && rock.worldZ - distanceTraveled < RockWarningMaxDistance)
                {
                    flashAmount = warningBrightness;
                    break;
                }
            }

            mainShader.setFloat("hitFlash", flashAmount);
            objectTransform.SetPos(glm::vec3(LANE_POSITIONS[lane], TileYOffset, tileZ));
            objectTransform.SetRot(glm::vec3(0.0f));
            objectTransform.SetScale(glm::vec3(TileLength));
            mainShader.Update(objectTransform, camera);
            trackTileMesh.draw();
        }
    }

    mainShader.setFloat("hitFlash", 0.0f);
}

// Draws all active collectible boxes. Collected boxes play a float-up animation
// before being removed; uncollected boxes spin continuously.
void GameRenderer::renderBoxes(const std::vector<BoxObject>& activeBoxes, float boxSpinAngle)
{
    // Draw scales for the three box size variants (small, medium, large)
    static const float BOX_DRAW_SCALES[3] = { 1.5f, 2.5f, 3.5f };

    for (auto& box : activeBoxes)
    {
        if (box.collected && box.collectAnimTimer <= 0.0f) continue;

        Mesh& boxMesh  = boxMeshVariants[box.meshVariant];
        float boxScale = BOX_DRAW_SCALES[box.meshVariant];
        float yPosition = 0.4f;

        // When collected the box floats upward and grows slightly over the animation duration
        if (box.collected && box.collectAnimTimer > 0.0f)
        {
            float animProgress = 1.0f - box.collectAnimTimer / CollectAnimDuration;
            yPosition += animProgress * CollectFloatHeight;
            boxScale  *= (1.0f + animProgress * CollectScaleBoost);
        }

        objectTransform.SetPos(glm::vec3(LANE_POSITIONS[box.lane], yPosition, box.worldZ));
        objectTransform.SetRot(glm::vec3(0.0f, boxSpinAngle, 0.0f));
        objectTransform.SetScale(glm::vec3(boxScale));
        mainShader.Update(objectTransform, camera);

        if (boxMesh.hasTexture()) boxMesh.bindTexture(0);
        boxMesh.draw();
    }
}

// Draws all uncollected rocks as static obstacles on the track.
void GameRenderer::renderRocks(const std::vector<BoxObject>& activeRocks)
{
    for (auto& rock : activeRocks)
    {
        if (rock.collected) continue;

        objectTransform.SetPos(glm::vec3(LANE_POSITIONS[rock.lane], RockYPosition, rock.worldZ));
        objectTransform.SetRot(glm::vec3(0.0f));
        objectTransform.SetScale(glm::vec3(RockScale));
        mainShader.Update(objectTransform, camera);

        if (rockMesh.hasTexture()) rockMesh.bindTexture(0);
        rockMesh.draw();
    }
}

// Draws all active one-up heart pickups. Uses the same float-up animation as boxes.
void GameRenderer::renderOneUps(const std::vector<OneUpObject>& activeOneUps, float boxSpinAngle)
{
    for (auto& oneUp : activeOneUps)
    {
        if (oneUp.collected && oneUp.collectAnimTimer <= 0.0f) continue;

        float heartScale = HeartScale;
        float yPosition  = HeartYPosition;

        // Same float-up animation as collectible boxes
        if (oneUp.collected && oneUp.collectAnimTimer > 0.0f)
        {
            float animProgress = 1.0f - oneUp.collectAnimTimer / CollectAnimDuration;
            yPosition  += animProgress * CollectFloatHeight;
            heartScale *= (1.0f + animProgress * CollectScaleBoost);
        }

        objectTransform.SetPos(glm::vec3(LANE_POSITIONS[oneUp.lane], yPosition, oneUp.worldZ));
        objectTransform.SetRot(glm::vec3(0.0f, boxSpinAngle, 0.0f));
        objectTransform.SetScale(glm::vec3(heartScale));
        mainShader.Update(objectTransform, camera);

        if (heartMesh.hasTexture()) heartMesh.bindTexture(0);
        heartMesh.draw();
    }
}

// Draws fence segments along the right edge of the track across the visible range.
void GameRenderer::renderFence(float distanceTraveled)
{
    mainShader.setFloat("hitFlash", 0.0f);
    mainShader.setVec3("colorMult", glm::vec3(1.0f, 1.0f, 1.0f));
    mainShader.setVec2("uvOffset", glm::vec2(0.0f, 0.0f));

    if (fenceMesh.hasTexture()) fenceMesh.bindTexture(0);

    // The fence model is 1 unit wide and scaled uniformly, so segment length equals scale
    const float FenceSegmentLength = FenceScale;

    int firstSegment = (int)((distanceTraveled - FenceDrawRange) / FenceSegmentLength);
    int lastSegment  = (int)((distanceTraveled + FenceDrawRange) / FenceSegmentLength);

    for (int seg = firstSegment; seg <= lastSegment; seg++)
    {
        float segZ = seg * FenceSegmentLength;

        objectTransform.SetPos(glm::vec3(FenceXPosition, 0.0f, segZ));
        objectTransform.SetRot(glm::vec3(0.0f, glm::radians(90.0f), 0.0f));
        objectTransform.SetScale(glm::vec3(FenceScale));
        mainShader.Update(objectTransform, camera);
        fenceMesh.draw();
    }
}

// Draws billboard bushes in two rows alongside the track.
// Each bush is a flat quad rotated to face the camera.
// Positions are derived deterministically from the index so they never shift between frames.
void GameRenderer::renderBushes(float distanceTraveled)
{
    struct BushRowConfig
    {
        float xPosition;
        float spacing;
        float baseScale;
    };

    const BushRowConfig bushRows[] = {
        {  9.0f,  5.5f, 3.0f },  // inner row just behind the fence
        { 13.0f,  6.5f, 4.0f },  // outer row further from the track
    };

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    mainShader.setFloat("hitFlash", 0.0f);

    for (int rowIndex = 0; rowIndex < 2; rowIndex++)
    {
        float rowX      = bushRows[rowIndex].xPosition;
        float spacing   = bushRows[rowIndex].spacing;
        float baseScale = bushRows[rowIndex].baseScale;

        int firstBushIndex = (int)((distanceTraveled - BushDrawBackRange)    / spacing) - 1;
        int lastBushIndex  = (int)((distanceTraveled + BushDrawForwardRange) / spacing) + 1;

        for (int bushIndex = firstBushIndex; bushIndex <= lastBushIndex; bushIndex++)
        {
            // Derive position and scale from the index so they remain stable across frames
            int positionSeed = (bushIndex * 7  + rowIndex * 13) & 0x7FFF;
            int offsetSeed   = (bushIndex * 31 + rowIndex * 17) & 0x7FFF;

            float bushZ     = bushIndex * spacing + (offsetSeed % 100) / 100.0f * spacing * 0.45f;
            float bushScale = baseScale + (positionSeed % 100) / 100.0f * 1.2f;

            // Only use the side-to-side angle to face the camera.
            // Using the forward distance too would make the bush spin as it scrolls past.
            float faceCameraAngle = atan2f(camera.getPos().x - rowX, 0.0f);

            objectTransform.SetPos(glm::vec3(rowX, 0.0f, bushZ));
            objectTransform.SetRot(glm::vec3(0.0f, faceCameraAngle, 0.0f));
            objectTransform.SetScale(glm::vec3(bushScale));
            mainShader.Update(objectTransform, camera);

            bushTextures[positionSeed % 2].Bind(0);
            bushBillboardMesh.draw();
        }
    }

    glDisable(GL_BLEND);
}

// Draws pine trees in four rows at increasing distances from the track.
// Tree positions, scales, and rotations are deterministic per index so they never shift.
void GameRenderer::renderTrees(float distanceTraveled)
{
    const float PI = 3.14159265f;

    struct TreeRowConfig
    {
        float xPosition;
        float spacing;
        float baseScale;
    };

    const TreeRowConfig treeRows[] = {
        { 18.0f,  7.0f, 6.0f },  // dense row close behind the fence
        { 30.0f,  7.0f, 6.0f },  // dense row further back
        { 24.0f, 12.0f, 7.0f },  // scattered row mid-distance
        { 36.0f, 12.0f, 7.0f },  // scattered row far background
    };

    mainShader.setFloat("hitFlash", 0.0f);

    for (int rowIndex = 0; rowIndex < 4; rowIndex++)
    {
        float rowX      = treeRows[rowIndex].xPosition;
        float spacing   = treeRows[rowIndex].spacing;
        float baseScale = treeRows[rowIndex].baseScale;

        int firstTreeIndex = (int)((distanceTraveled - TreeDrawBackRange)    / spacing) - 1;
        int lastTreeIndex  = (int)((distanceTraveled + TreeDrawForwardRange) / spacing) + 1;

        for (int treeIndex = firstTreeIndex; treeIndex <= lastTreeIndex; treeIndex++)
        {
            int positionSeed = (treeIndex * 11 + rowIndex * 19) & 0x7FFF;
            int offsetSeed   = (treeIndex * 37 + rowIndex * 23) & 0x7FFF;

            float treeZ     = treeIndex * spacing + (offsetSeed % 100) / 100.0f * spacing * 0.4f;
            float treeScale = baseScale + (positionSeed % 100) / 100.0f * 2.5f;

            // Use one of 8 evenly spaced Y rotations (every 45 degrees) for visual variety
            float yRotation = (positionSeed % 8) * (PI / 4.0f);

            // Alternate between the two pine tree models; pineTreeMeshB is shorter,
            // so scale it up by the height ratio so both appear the same visual height
            bool useModelB   = (positionSeed % 2 == 0);
            float finalScale = useModelB ? treeScale : treeScale * PineBModelScaleRatio;

            // Vary brightness slightly so trees don't all look identical without hue-shifting the trunk
            float tintBlend = (positionSeed % 100) / 100.0f;
            float brightness = glm::mix(0.88f, 1.08f, tintBlend);
            mainShader.setVec3("colorMult", glm::vec3(brightness));

            objectTransform.SetPos(glm::vec3(rowX, 0.0f, treeZ));
            objectTransform.SetRot(glm::vec3(0.0f, yRotation, 0.0f));
            objectTransform.SetScale(glm::vec3(finalScale));
            mainShader.Update(objectTransform, camera);

            Mesh& treeMesh = useModelB ? pineTreeMeshB : pineTreeMesh;
            if (treeMesh.hasTexture()) treeMesh.bindTexture(0);
            treeMesh.draw();
        }
    }

    // Reset the colour multiplier so subsequent objects are drawn at full colour
    mainShader.setVec3("colorMult", glm::vec3(1.0f, 1.0f, 1.0f));
}

// Draws the player train with a Z-axis tilt and an optional red hit flash.
void GameRenderer::renderTrain(glm::vec3 trainPosition, float trainLeanAngle, float hitFlashAmount)
{
    mainShader.setFloat("hitFlash", hitFlashAmount);

    objectTransform.SetPos(trainPosition);
    objectTransform.SetRot(glm::vec3(0.0f, 0.0f, trainLeanAngle));
    objectTransform.SetScale(glm::vec3(TrainScale));
    mainShader.Update(objectTransform, camera);

    if (trainMesh.hasTexture()) trainMesh.bindTexture(0);
    trainMesh.draw();

    mainShader.setFloat("hitFlash", 0.0f);
}

// Draws the score, lives, combo, double-points indicator, floating score popups,
// game over screen (with lifetime stats), and pause menu.
void GameRenderer::renderHUD(Display& display, const GameRenderState& state)
{
    glm::vec4 whiteColor (1.0f, 1.0f, 1.0f, 1.0f);
    glm::vec4 yellowColor(1.0f, 0.92f, 0.3f, 1.0f);
    glm::vec4 redColor   (1.0f, 0.3f,  0.3f, 1.0f);

    textRenderer.draw("Score: " + std::to_string(state.score), HudLeftMargin, HudScoreLineY, 1.0f, whiteColor);
    textRenderer.draw("Lives: " + std::to_string(state.lives) + " / " + std::to_string(state.maxLives),
        HudLeftMargin, HudLivesLineY, 1.0f, state.lives > 1 ? whiteColor : redColor);

    if (state.combo >= 3)
    {
        int       multiplier = (state.combo >= 6) ? 3 : 2;
        glm::vec4 comboColor = (multiplier >= 3)
            ? glm::vec4(1.0f, 0.85f, 0.0f, 1.0f)
            : glm::vec4(1.0f, 0.60f, 0.05f, 1.0f);
        textRenderer.draw("Combo x" + std::to_string(multiplier) + "!", HudLeftMargin, HudComboLineY, 1.0f, comboColor);
    }

    // Double-points indicator: shown below the combo slot, pulses cyan
    if (state.doublePointsTimer > 0.0f)
    {
        int   secsLeft = (int)ceilf(state.doublePointsTimer);
        float pulse    = 0.75f + 0.25f * sinf(SDL_GetTicks() * 0.006f);
        glm::vec4 bonusColor(0.2f * pulse, 1.0f * pulse, 0.85f * pulse, 1.0f);
        textRenderer.draw("2X POINTS  " + std::to_string(secsLeft) + "s",
            HudLeftMargin, HudDoubleLineY, 1.0f, bonusColor);
    }

    // Advance and draw each score popup; they drift upward and fade out over their lifetime
    for (auto& popup : *state.scorePopups)
    {
        popup.timeRemaining -= state.deltaTime;
        popup.screenY       -= PopupDriftSpeed * state.deltaTime;

        float     fadeAlpha  = glm::clamp(popup.timeRemaining / 0.9f, 0.0f, 1.0f);
        float     popupScale = 0.9f + (1.0f - fadeAlpha) * PopupScaleRange;
        glm::vec4 popupColor = popup.color;
        popupColor.a         = fadeAlpha;
        textRenderer.draw(popup.text, popup.screenX, popup.screenY, popupScale, popupColor);
    }
    state.scorePopups->erase(
        std::remove_if(state.scorePopups->begin(), state.scorePopups->end(),
            [](const ScorePopup& p) { return p.timeRemaining <= 0.0f; }),
        state.scorePopups->end()
    );

    if (state.isGameOver)
    {
        renderDarkOverlay(GameOverOverlayAlpha);
        float centerX = display.getWidth()  * 0.5f;
        float centerY = display.getHeight() * 0.5f;
        float leftX   = centerX - GameOverPanelOffsetX;

        glm::vec4 selectedColor(1.0f, 0.92f, 0.3f, 1.0f);

        textRenderer.draw("GAME OVER",                                     leftX, centerY - 80.0f,  1.8f,  redColor);
        textRenderer.draw("Score: " + std::to_string(state.score),         leftX, centerY +  5.0f,  1.0f,  yellowColor);
        textRenderer.draw("Best:  " + std::to_string(state.highScore),     leftX, centerY + 47.0f,  0.85f, yellowColor);
        textRenderer.draw("Lifetime: " + std::to_string((int)state.totalDistanceAllTime) + "m  ("
            + std::to_string(state.totalSessions) + " runs)",               leftX, centerY + 85.0f,  0.75f, glm::vec4(0.7f, 0.7f, 0.8f, 1.0f));
        textRenderer.draw("> Restart",   leftX, centerY + 130.0f, 1.0f, state.gameOverMenuSelection == 0 ? selectedColor : whiteColor);
        textRenderer.draw("> Main Menu", leftX, centerY + 175.0f, 1.0f, state.gameOverMenuSelection == 1 ? selectedColor : whiteColor);
        textRenderer.draw("> Quit",      leftX, centerY + 220.0f, 1.0f, state.gameOverMenuSelection == 2 ? selectedColor : whiteColor);
        textRenderer.draw("UP / DOWN to select   ENTER to confirm",
                                         leftX, centerY + 265.0f, 0.55f, glm::vec4(0.8f, 0.8f, 0.8f, 0.7f));
    }

    if (state.isPaused)
    {
        renderDarkOverlay(PauseOverlayAlpha);
        float centerX = display.getWidth()  * 0.5f;
        float centerY = display.getHeight() * 0.5f;
        float leftX   = centerX - GameOverPanelOffsetX;

        glm::vec4 selectedColor(1.0f, 0.92f, 0.3f, 1.0f);
        glm::vec4 normalColor  (1.0f, 1.0f,  1.0f, 1.0f);

        textRenderer.draw("PAUSED",      leftX, centerY - 60.0f,  1.8f, whiteColor);
        textRenderer.draw("> Resume",    leftX, centerY + 20.0f,  1.0f, state.pauseMenuSelection == 0 ? selectedColor : normalColor);
        textRenderer.draw("> Main Menu", leftX, centerY + 65.0f,  1.0f, state.pauseMenuSelection == 1 ? selectedColor : normalColor);
        textRenderer.draw("> Quit",      leftX, centerY + 110.0f, 1.0f, state.pauseMenuSelection == 2 ? selectedColor : normalColor);
        textRenderer.draw("UP / DOWN to select   ENTER to confirm", leftX, centerY + 160.0f, 0.6f, glm::vec4(0.8f, 0.8f, 0.8f, 0.7f));
    }
}

// Draws screen-space rain streaks as a transparent overlay using the sky shader's rain branch.
void GameRenderer::renderRain(float intensity, float time, float aspectRatio)
{
    if (intensity <= 0.0f) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    skyShader.Bind();
    skyShader.setFloat("overlayAlpha",  0.0f);
    skyShader.setFloat("rainMode",      1.0f);
    skyShader.setFloat("rainIntensity", intensity);
    skyShader.setFloat("rainTime",      time);
    skyShader.setFloat("aspectRatio",   aspectRatio);
    overlayQuadMesh.draw();
    skyShader.setFloat("rainMode", 0.0f);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

// Draws a full-screen semi-transparent dark quad over the scene using the sky shader.
void GameRenderer::renderDarkOverlay(float alpha)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    skyShader.Bind();
    skyShader.setFloat("overlayAlpha", alpha);
    skyShader.setFloat("rainMode",     0.0f);
    overlayQuadMesh.draw();
    skyShader.setFloat("overlayAlpha", 0.0f);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

// Menu camera: positioned to the left and behind the train at a low three-quarter angle
// so trees and fence fill the right side of the frame.
static const float MenuCamX         = -6.0f;
static const float MenuCamY         =  4.0f;
static const float MenuCamZ         =  1.0f;
static const float MenuLookX        =  0.0f;
static const float MenuLookY        =  1.2f;
static const float MenuLookZ        = 15.0f;
// A very slow lateral and vertical sway (period about 21 seconds) keeps the shot feeling alive.
static const float MenuSwaySpeedX   = 0.30f;  // radians per second
static const float MenuSwaySpeedY   = 0.18f;  // radians per second, offset so X and Y never sync
static const float MenuSwayAmplX    = 0.80f;  // world units
static const float MenuSwayAmplY    = 0.25f;  // world units
// Train Z position in the menu shot
static const float MenuTrainZ       = 6.0f;

// Renders a cinematic title screen using the same environment assets as gameplay.
// No HUD or gameplay objects are shown; the camera is fixed at a low three-quarter angle.
void GameRenderer::renderMenuFrame(Display& display, float menuTime)
{
    // Apply the slow sway to give the static shot a breathing quality
    float swayX = sinf(menuTime * MenuSwaySpeedX) * MenuSwayAmplX;
    float swayY = sinf(menuTime * MenuSwaySpeedY) * MenuSwayAmplY;
    camera.setPos(glm::vec3(MenuCamX + swayX, MenuCamY + swayY, MenuCamZ));
    camera.setLook(glm::vec3(MenuLookX, MenuLookY, MenuLookZ));

    display.clearDisplay(ClearDayR, ClearDayG, ClearDayB, 1.0f);

    float aspectRatio = display.getWidth() / display.getHeight();
    renderSky(0.0f, aspectRatio);

    glm::vec3 menuTrainPos(0.0f, 0.0f, MenuTrainZ);

    mainShader.Bind();
    mainShader.setFloat("hitFlash",      0.0f);
    mainShader.setVec3 ("colorMult",     glm::vec3(1.0f));
    mainShader.setInt  ("diffuse",       0);
    mainShader.setInt  ("normalMap",     1);
    mainShader.setInt  ("hasNormalMap",  0);
    mainShader.setFloat("rainIntensity", 0.0f);
    setLightingUniforms(0.0f, menuTrainPos);

    // Draw tiles and fence in three separate passes so they cover the full visible distance.
    renderGround(0.0f);

    static const std::vector<BoxObject> noRocks;
    for (float passCenter = 0.0f; passCenter <= 180.0f; passCenter += 60.0f)
    {
        renderFence(passCenter);
        renderTrackTiles(passCenter, noRocks);
    }

    renderBushes(30.0f);
    renderTrees(30.0f);
    renderTrain(menuTrainPos, 0.0f, 0.0f);

    renderMenuUI(display, menuTime);

    display.setTitle("Cargo Collector");
    display.swapBuffer();
}

// Draws the game title and pulsing prompt text over the menu scene.
void GameRenderer::renderMenuUI(Display& display, float menuTime)
{
    float screenW = display.getWidth();
    float screenH = display.getHeight();

    // Title anchored toward the right but far enough left to stay fully on-screen
    float titleX = screenW * 0.30f;
    float titleY = screenH * 0.18f;

    glm::vec4 titleColor (1.0f, 0.90f, 0.20f, 1.0f);
    textRenderer.draw("CARGO COLLECTOR", titleX, titleY, 2.0f, titleColor);

    // Prompt sits in the open sky on the right, beneath the right half of the title
    float pulseAlpha = 0.65f + 0.35f * sinf(menuTime * 2.5f);
    glm::vec4 promptColor(1.0f, 1.0f, 1.0f, pulseAlpha);
    textRenderer.draw("Press ENTER to Play", screenW * 0.6f, titleY + 70.0f, 0.88f, promptColor);
}
