#pragma once
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>

// Handles all game audio: generated beep sounds and background music loaded from a file.
class AudioSystem
{
public:
    AudioSystem();
    ~AudioSystem();

    // Opens the audio device, initialises SDL_mixer, and loads background music
    void init();

    // Closes the audio device and frees music resources
    void shutdown();

    // Removes any sound effects that are still waiting to play
    void clearQueue();

    // Generates and plays a simple beep sound effect
    void playBeep(float frequency, float durationSeconds, float volume = 0.5f);

    // Plays the three-note jingle that plays when the game ends
    void playGameOverJingle();

    // Starts looping the background music (does nothing if it failed to load)
    void playMusic();

    // Stops the background music
    void stopMusic();

    // Sets background music volume: 0 = silent, 128 = full
    void setMusicVolume(int volume);

    // Pauses / resumes background music without losing the playback position
    void pauseMusic();
    void resumeMusic();

    // Sets the rain ambient sound volume (0 = silent, 1 = full); starts/stops the loop as needed
    void setRainVolume(float intensity);

private:
    SDL_AudioDeviceID audioDevice;
    Mix_Music*        bgMusic;
    Mix_Chunk*        rainChunk;   // the rain sound clip that loops while it's raining
    int               rainChannel; // which audio channel the rain is using (-1 means not playing)
};
