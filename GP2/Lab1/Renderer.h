#pragma once
#include "Data.h"
#include "Display.h"
#include "Shader.h"
#include "Mesh.h"
#include "Texture.h"
#include "Camera.h"
#include "Transform.h"
#include "Text.h"

// Owns all rendering resources (meshes, textures, shaders, camera) and
// draws the entire scene each frame using data passed in from the game logic.
class GameRenderer
{
public:
    GameRenderer();

    // Loads all meshes, textures, and shaders, and sets up the camera.
    void init(Display& display);

    // Draws the complete scene for one game frame.
    void renderFrame(Display& display, const GameRenderState& state);

    // Draws the cinematic main menu scene with title and prompt text.
    void renderMenuFrame(Display& display, float menuTime);

    // Returns the camera so MainGame can move it during debug input.
    Camera& getCamera() { return camera; }

private:
    // Draws the sky gradient quad behind all geometry.
    void renderSky(float dayNightProgress, float aspectRatio);

    // Draws the grass and mud ground planes.
    void renderGround(float distanceTraveled);

    // Draws the track tiles across the visible range, with lane warning flashes.
    void renderTrackTiles(float distanceTraveled, const std::vector<BoxObject>& activeRocks);

    // Draws all active collectible boxes, including their collect animations.
    void renderBoxes(const std::vector<BoxObject>& activeBoxes, float boxSpinAngle);

    // Draws all active rock obstacles on the track.
    void renderRocks(const std::vector<BoxObject>& activeRocks);

    // Draws all active one-up heart pickups, including their collect animations.
    void renderOneUps(const std::vector<OneUpObject>& activeOneUps, float boxSpinAngle);

    // Draws fence segments along the right edge of the track.
    void renderFence(float distanceTraveled);

    // Draws billboard bushes alongside the track.
    void renderBushes(float distanceTraveled);

    // Draws rows of pine trees at the sides of the track.
    void renderTrees(float distanceTraveled);

    // Draws the player train with a tilt and optional red hit flash.
    void renderTrain(glm::vec3 trainPosition, float trainLeanAngle, float hitFlashAmount);

    // Draws the HUD: score, lives, combo, popups, game over screen, and pause menu.
    void renderHUD(Display& display, const GameRenderState& state);

    // Draws screen-space rain streaks as a transparent overlay.
    void renderRain(float intensity, float time, float aspectRatio);

    // Draws a full-screen semi-transparent dark quad over the scene.
    void renderDarkOverlay(float alpha);

    // Draws the title and pulsing prompt text on the menu screen.
    void renderMenuUI(Display& display, float menuTime);

    // Uploads the hemisphere lighting and fog uniforms to the main shader.
    void setLightingUniforms(float dayNightProgress, glm::vec3 trainPosition);

    Camera    camera;
    Transform objectTransform;  // Reused to position every object in the scene

    Mesh skyQuadMesh;        // Full-screen background quad for the sky gradient
    Mesh overlayQuadMesh;    // Full-screen darkening quad for the game over screen
    Mesh grassMesh;          // Far-side grass plane beyond the mud strip
    Mesh nearsideGrassMesh;  // Near-side strip between the camera and the mud
    Mesh mudStripMesh;       // Narrower mud strip over the track bed
    Mesh trackTileMesh;      // Single track tile drawn many times to fill each lane
    Mesh trainMesh;
    Mesh boxMeshVariants[3]; // Three sizes of box, all using the same obj but drawn at different scales
    Mesh rockMesh;
    Mesh heartMesh;          // The one-up pickup
    Mesh fenceMesh;
    Mesh bushBillboardMesh;  // Flat quad rotated to face the camera for each bush
    Mesh pineTreeMesh;       // tree-pine.obj, model height 2.0 units
    Mesh pineTreeMeshB;      // tree-pine2.obj, model height 0.83 units, scaled up to match pineTreeMesh

    Texture grassTexture;
    Texture mudTexture;
    Texture groundNormalMap;
    Texture bushTextures[2];

    Shader mainShader;
    Shader skyShader;

    TextRenderer textRenderer;
};
