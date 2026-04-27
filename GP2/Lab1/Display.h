#pragma once
#include <SDL/SDL.h>
#include <GL\glew.h>
#include <iostream>
#include <string>

// Creates and manages the SDL window and OpenGL context.
// Provides helpers for clearing the screen and showing the finished frame.
class Display
{
public:
    Display();
    ~Display();

    // Opens the window and initialises the OpenGL context.
    void initDisplay();

    // Shows the finished frame on screen.
    void swapBuffer();

    // Clears the colour and depth buffers with the specified RGBA colour.
    void clearDisplay(float r, float g, float b, float a);

    // Returns the width of the window in pixels.
    float getWidth();

    // Returns the height of the window in pixels.
    float getHeight();

    // Updates the window title bar text.
    void  setTitle(const std::string& title);

private:
    // Prints an error message and terminates the application.
    void returnError(const std::string& errorString);

    SDL_GLContext glContext;  // OpenGL rendering context tied to the window
    SDL_Window*   sdlWindow;  // pointer to the SDL window
    float         screenWidth;
    float         screenHeight;
};
