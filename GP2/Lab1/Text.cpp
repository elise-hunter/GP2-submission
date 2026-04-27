// Bitmap font renderer using stb_truetype.
// Bakes a TTF font into a greyscale GL texture at init time,
// then draws screen space quads for each character at draw time.

// Compile implementation with C linkage so it matches the extern "C"
// declarations in Text.h, same pattern as stb_image.c/.h.
extern "C" {
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#undef STB_TRUETYPE_IMPLEMENTATION  // prevent re-compilation when header is included again
}

#include "Text.h"
#include <fstream>
#include <vector>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

TextRenderer::TextRenderer()
    : fontTexture(0), VAO(0), VBO(0), screenW(0), screenH(0)
{}

TextRenderer::~TextRenderer()
{
    glDeleteTextures(1, &fontTexture);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

void TextRenderer::init(const char* fontPath, float fontSize, float screenW_, float screenH_)
{
    screenW = screenW_;
    screenH = screenH_;

    // Read font file into memory
    std::ifstream file(fontPath, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        std::cerr << "TextRenderer: could not open font: " << fontPath << std::endl;
        return;
    }
    size_t sz = (size_t)file.tellg();
    file.seekg(0);
    std::vector<unsigned char> ttfBuf(sz);
    file.read(reinterpret_cast<char*>(ttfBuf.data()), sz);

    // Bake glyph bitmaps into a greyscale atlas (ASCII 32-127)
    std::vector<unsigned char> bitmap(ATLAS * ATLAS);
    stbtt_BakeFontBitmap(ttfBuf.data(), 0, fontSize,
                         bitmap.data(), ATLAS, ATLAS,
                         32, 96, cdata);

    // Upload atlas as a single-channel texture
    glGenTextures(1, &fontTexture);
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, ATLAS, ATLAS, 0,
                 GL_RED, GL_UNSIGNED_BYTE, bitmap.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Dynamic VBO: 6 vertices by 4 floats (xy + uv) per character quad
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    textShader.init("..\\res\\text.vert", "..\\res\\text.frag");
}

// Internal: draws without drop shadow
void TextRenderer::drawRaw(const std::string& text, float x, float y, float scale, glm::vec4 color)
{
    if (!fontTexture) return;

    textShader.Bind();

    // Orthographic projection: y=0 at top, y=screenH at bottom
    glm::mat4 proj = glm::ortho(0.0f, screenW, screenH, 0.0f);
    textShader.setMat4("projection", proj);
    textShader.setVec4("textColor", color);
    textShader.setInt("fontTex", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    glBindVertexArray(VAO);

    // Cursor starts at (0,0) in "baked" space; we scale + offset to screen
    float cx = 0.0f, cy = 0.0f;

    for (char c : text)
    {
        if (c < 32 || c >= 128) { cx += 10.0f; continue; }

        stbtt_aligned_quad q;
        stbtt_GetBakedQuad(cdata, ATLAS, ATLAS, c - 32, &cx, &cy, &q, 0);

        // q.x0/y0/x1/y1 are in "baked space" from cursor origin 0,0
        // Scale and offset into screen space
        float sx0 = x + q.x0 * scale, sy0 = y + q.y0 * scale;
        float sx1 = x + q.x1 * scale, sy1 = y + q.y1 * scale;

        float verts[6][4] = {
            { sx0, sy0, q.s0, q.t0 },
            { sx0, sy1, q.s0, q.t1 },
            { sx1, sy1, q.s1, q.t1 },
            { sx0, sy0, q.s0, q.t0 },
            { sx1, sy1, q.s1, q.t1 },
            { sx1, sy0, q.s1, q.t0 },
        };

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void TextRenderer::draw(const std::string& text, float x, float y, float scale, glm::vec4 color)
{
    if (!fontTexture) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    // Drop shadow: 2px offset, dark semi-transparent
    drawRaw(text, x + 2.0f * scale, y + 2.0f * scale, scale, glm::vec4(0.0f, 0.0f, 0.0f, 0.6f));
    drawRaw(text, x, y, scale, color);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}
