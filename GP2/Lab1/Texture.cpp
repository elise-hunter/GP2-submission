// Loads an image from disk and uploads it to the GPU as a texture.

#include "Texture.h"
#include "stb_image.h"
#include <cassert>
#include <iostream>

// OpenGL provides texture slots numbered 0 through 31; no valid unit exceeds this
static const unsigned int MaxTextureUnit = 31;

// Set to zero so glDeleteTextures ignores it safely if init() is never called.
Texture::Texture()
{
    textureHandler = 0;
}

// Frees the GPU texture object.
Texture::~Texture()
{
    glDeleteTextures(1, &textureHandler);
}

// Returns true if x is a power of two (e.g. 2, 4, 8, 16...).
static bool isPowerOfTwo(int x)
{
    return x > 0 && (x & (x - 1)) == 0;
}

// Reads the image at fileName, converts it to RGBA, and uploads it to the GPU.
// Generates mipmaps and sets up filtering after the image is uploaded.
void Texture::init(const std::string& fileName)
{
    int width, height, numComponents;

    // Force RGBA regardless of the source format so the shader always receives 4 channels
    unsigned char* imageData = stbi_load(fileName.c_str(), &width, &height, &numComponents, 4);

    if (imageData == NULL)
    {
        std::cerr << "Texture load failed: " << fileName << std::endl;
        return;
    }

    glGenTextures(1, &textureHandler);
    glBindTexture(GL_TEXTURE_2D, textureHandler);

    // Textures that are not a power of two in size cannot tile reliably on all hardware,
    // so clamp them to their edge instead of repeating.
    GLenum wrapMode = (isPowerOfTwo(width) && isPowerOfTwo(height)) ? GL_REPEAT : GL_CLAMP_TO_EDGE;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);

    // Mipmaps need to be generated after the image data is uploaded.
    glGenerateMipmap(GL_TEXTURE_2D);

    // Smooth blending between mipmap levels for distant surfaces,
    // and standard bilinear for close-up surfaces.
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(imageData);
}

// Activates the given texture slot and binds this texture to it.
void Texture::Bind(unsigned int unit)
{
    assert(unit <= MaxTextureUnit);

    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, textureHandler);
}
