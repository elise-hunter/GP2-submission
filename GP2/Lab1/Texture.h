#pragma once
#include <string>
#include <GL\glew.h>

// Loads a single image from disk and manages its GPU texture object.
// Copy and assignment are disabled because the GPU resource is owned by this object.
class Texture
{
public:
    Texture();
    ~Texture();
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    // Activates this texture on the given slot (0-31).
    void Bind(unsigned int unit);

    // Loads the image at fileName and uploads it to the GPU as an RGBA texture.
    void init(const std::string& fileName);

private:
    GLuint textureHandler;  // GPU texture handle; 0 means nothing loaded yet
};
