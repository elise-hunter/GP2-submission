// Creates the window and OpenGL context, and presents each rendered frame.

#include "Display.h"
#include <cstdlib>

// Default window dimensions in pixels
static const float WindowWidth  = 1024.0f;
static const float WindowHeight = 768.0f;

// Colour precision requested for the window
static const int ColorChannelBits = 8;

// Depth precision; 16 bits is enough for this game's depth range
static const int DepthBufferBits = 16;

// Sets default window dimensions; the actual window is not created until initDisplay() is called.
Display::Display()
{
    sdlWindow    = nullptr;
    screenWidth  = WindowWidth;
    screenHeight = WindowHeight;
}

// Destroys the context first, then the window, then shuts down SDL.
// The context has to go before the window, otherwise it'd be pointing at memory that no longer exists.
Display::~Display()
{
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(sdlWindow);
    SDL_Quit();
}

// Returns the window width in pixels.
float Display::getWidth()  { return screenWidth; }

// Returns the window height in pixels.
float Display::getHeight() { return screenHeight; }

// Updates the text shown in the window title bar.
void  Display::setTitle(const std::string& title) { SDL_SetWindowTitle(sdlWindow, title.c_str()); }

// Prints the error string to stderr and exits the application.
void Display::returnError(const std::string& errorString)
{
    std::cerr << "Error: " << errorString << std::endl;
    SDL_Quit();
    exit(1);
}

// Shows the finished frame on screen.
void Display::swapBuffer()
{
    SDL_GL_SwapWindow(sdlWindow);
}

// Fills the colour and depth buffers with the specified RGBA colour.
void Display::clearDisplay(float r, float g, float b, float a)
{
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

// Initialises SDL and GLEW, creates the window, and configures OpenGL state.
void Display::initDisplay()
{
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
        returnError(SDL_GetError());

    // Request colour precision from the driver
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   ColorChannelBits);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, ColorChannelBits);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  ColorChannelBits);

    // Without a depth buffer, depth testing would not work at all.
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, DepthBufferBits);

    // Double buffering means the player never sees a partially drawn frame.
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    sdlWindow = SDL_CreateWindow("Game Window",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        (int)screenWidth, (int)screenHeight,
        SDL_WINDOW_OPENGL);

    if (sdlWindow == nullptr)
        returnError("window failed to create");

    glContext = SDL_GL_CreateContext(sdlWindow);

    if (glContext == nullptr)
        returnError("SDL_GL context failed to create");

    GLenum error = glewInit();
    if (error != GLEW_OK)
        returnError("GLEW failed to initialise");

    // Without depth testing, objects drawn later would appear on top regardless of distance.
    glEnable(GL_DEPTH_TEST);

    // Skip drawing the back faces of meshes as the camera can never see them.
    glEnable(GL_CULL_FACE);

    glClearColor(0.0f, 1.0f, 1.0f, 1.0f);
}
