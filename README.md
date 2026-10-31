# Cargo Collector

A 3D endless runner made in C++ and OpenGL. You control a train on three rails. Collect cargo, avoid rocks, and try to beat your high score.

**[Play it on itch.io](https://elisehunter.itch.io/cargo-collector)**

## About

I made this game for the Games Programming 2 module at Glasgow Caledonian University (January - May 2026). It got a grade of 90%. (˶' ꒳ '˶)

I wrote the game and the renderer myself in C++ and OpenGL. I did not use a game engine.

## Features

### Graphics

- **Lighting:** GLSL shaders with normal mapping and Blinn-Phong lighting.
- **Day-to-night cycle:** The sun changes to a moon, and the light colour and fog colour change with the time of day.
- **Procedural sky:** The sky is made in a fragment shader, with a sunset, a moon that rises, and three layers of stars that fade in at night.
- **Rain and fog:** Distance fog gets thicker when it rains, and a rain overlay makes it harder to see.
- **Model loading:** An OBJ/MTL loader that calculates smooth normals and tangents.
- **Text rendering:** HUD and menu text with stb_truetype.

### Gameplay

- Change rails to collect cargo and score points.
- A rail flashes red when a rock comes towards you.
- Each hit takes one life and ends your combo streak. Collect 1-UP hearts to get lives back.
- 2x Points event: Cargo is worth double points for a short time.
- The high score and lifetime stats save to a file.
- Sound effects are generated in code. Music and rain sounds play with SDL_mixer.

## Controls

| Key | Action |
| --- | --- |
| Up / Down arrows | Change rails |
| Enter | Start / OK |
| Escape | Pause |

## Tech stack

- C++
- OpenGL and GLSL
- SDL2 and SDL2_mixer (window, input and audio)
- GLEW (OpenGL loading)
- GLM (maths)
- stb_image and stb_truetype (textures and fonts)

## Credits

- 2D/3D assets and background music: [Kenney](https://kenney.itch.io/kenney-game-assets)
- Rain sound: [Ylmir](https://opengameart.org/content/rain-loopable) (OpenGameArt)
