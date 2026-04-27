// Sound effects are generated as sine waves and faded out at the end so they don't click.

#include "Audio.h"
#include <vector>
#include <cmath>

// Audio device configuration
static const int AudioSampleRate    = 44100;  // samples per second
static const int AudioBufferSamples = 512;    // SDL internal buffer size in samples

// Game-over jingle: three descending notes played in sequence
static const float JingleNote1Freq = 494.0f;  // B4
static const float JingleNote2Freq = 370.0f;  // F#4
static const float JingleNote3Freq = 247.0f;  // B3
static const float JingleNote1Dur  = 0.14f;   // seconds
static const float JingleNote2Dur  = 0.14f;
static const float JingleNote3Dur  = 0.28f;
static const float JingleVolume    = 0.55f;

// Constant used in the sine wave formula: 2 * PI
static const float TwoPi = 6.28318530f;

AudioSystem::AudioSystem() : audioDevice(0), bgMusic(nullptr),
    rainChunk(nullptr), rainChannel(-1) {}

// Calls shutdown to release the audio device when the object is destroyed.
AudioSystem::~AudioSystem()
{
    shutdown();
}

// Opens the SDL audio device for sound effects and initialises SDL_mixer for music.
void AudioSystem::init()
{
    SDL_AudioSpec desiredSpec{};
    desiredSpec.freq     = AudioSampleRate;
    desiredSpec.format   = AUDIO_S16SYS;
    desiredSpec.channels = 1;
    desiredSpec.samples  = AudioBufferSamples;
    desiredSpec.callback = nullptr;

    audioDevice = SDL_OpenAudioDevice(nullptr, 0, &desiredSpec, nullptr, 0);
    if (audioDevice)
        SDL_PauseAudioDevice(audioDevice, 0);

    // SDL_mixer runs on its own device, separate from the sound effect queue above
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 512) == 0)
        bgMusic = Mix_LoadMUS("..\\res\\music.ogg");

    rainChunk = Mix_LoadWAV("..\\res\\rain.ogg");
}

// Closes the SDL audio device and releases all mixer resources.
void AudioSystem::shutdown()
{
    if (audioDevice)
    {
        SDL_CloseAudioDevice(audioDevice);
        audioDevice = 0;
    }

    if (rainChannel >= 0) { Mix_HaltChannel(rainChannel); rainChannel = -1; }
    if (rainChunk)  { Mix_FreeChunk(rainChunk);  rainChunk  = nullptr; }

    Mix_HaltMusic();
    if (bgMusic) { Mix_FreeMusic(bgMusic); bgMusic = nullptr; }
    Mix_CloseAudio();
}

// Starts the background music track, looping indefinitely.
void AudioSystem::playMusic()
{
    if (bgMusic && !Mix_PlayingMusic())
        Mix_PlayMusic(bgMusic, -1);
}

// Stops the background music immediately.
void AudioSystem::stopMusic()
{
    Mix_HaltMusic();
}

// Pauses the music at its current position.
void AudioSystem::pauseMusic()
{
    if (Mix_PlayingMusic()) Mix_PauseMusic();
}

// Resumes the music from where it was paused.
void AudioSystem::resumeMusic()
{
    if (Mix_PausedMusic()) Mix_ResumeMusic();
}

// Sets the background music volume. value is clamped to [0, 128] by SDL_mixer.
void AudioSystem::setMusicVolume(int volume)
{
    Mix_VolumeMusic(volume);
}

// Discards any audio samples that are still waiting in the queue.
void AudioSystem::clearQueue()
{
    if (audioDevice)
        SDL_ClearQueuedAudio(audioDevice);
}

// Builds a sine wave tone and queues it for playback.
// It fades out at the end to avoid an audible click.
void AudioSystem::playBeep(float frequency, float durationSeconds, float volume)
{
    if (!audioDevice) return;

    int totalSamples = (int)(AudioSampleRate * durationSeconds);
    std::vector<Sint16> audioBuffer(totalSamples);

    for (int sampleIndex = 0; sampleIndex < totalSamples; sampleIndex++)
    {
        float time     = (float)sampleIndex / AudioSampleRate;
        float envelope = 1.0f - (time / durationSeconds);  // linear fade-out
        audioBuffer[sampleIndex] = (Sint16)(envelope * volume * 32767.0f
            * sinf(TwoPi * frequency * time));
    }

    SDL_QueueAudio(audioDevice, audioBuffer.data(), (Uint32)(audioBuffer.size() * sizeof(Sint16)));
}

// Starts, stops, or adjusts the volume of the looping rain ambient sound.
void AudioSystem::setRainVolume(float intensity)
{
    if (!rainChunk) return;

    if (intensity > 0.005f)
    {
        if (rainChannel < 0 || !Mix_Playing(rainChannel))
            rainChannel = Mix_PlayChannel(-1, rainChunk, -1);
        if (rainChannel >= 0)
            Mix_Volume(rainChannel, (int)(intensity * 80));
    }
    else if (rainChannel >= 0)
    {
        Mix_HaltChannel(rainChannel);
        rainChannel = -1;
    }
}

// Queues the three-note descending jingle that plays when the game ends.
void AudioSystem::playGameOverJingle()
{
    playBeep(JingleNote1Freq, JingleNote1Dur, JingleVolume);
    playBeep(JingleNote2Freq, JingleNote2Dur, JingleVolume);
    playBeep(JingleNote3Freq, JingleNote3Dur, JingleVolume);
}
