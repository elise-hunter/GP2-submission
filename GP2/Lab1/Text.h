#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <string>
#include "Shader.h"
extern "C" {
#include "stb_truetype.h"
}

class TextRenderer
{
public:
    TextRenderer();
    ~TextRenderer();

    // fontPath  : path to a .ttf file
    // fontSize  : pixels to bake at (acts as the "1x" size)
    // screenW/H : pixel dimensions of the window
    void init(const char* fontPath, float fontSize, float screenW, float screenH);

    // Draw text with a drop shadow.
    // x, y     : top-left baseline anchor in screen pixels (y increases down)
    // scale    : multiplier on the baked font size
    // color    : RGBA
    void draw(const std::string& text, float x, float y, float scale, glm::vec4 color);

private:
    Shader            textShader;
    GLuint            fontTexture;
    GLuint            VAO, VBO;
    stbtt_bakedchar   cdata[96]; // ASCII 32-127
    float             screenW, screenH;

    static const int  ATLAS = 512;

    void drawRaw(const std::string& text, float x, float y, float scale, glm::vec4 color);
};
