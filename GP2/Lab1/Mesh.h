#pragma once
#include <glm\glm.hpp>
#include <GL\glew.h>
#include <string>
#include "ObjLoader.h"
#include "Texture.h"

// Holds the per-vertex data sent to the GPU: position, texture coordinate, normal, and tangent.
struct Vertex
{
public:
    // Constructs a vertex with explicit position and UV, and optional normal and tangent.
    Vertex(const glm::vec3& pos, const glm::vec2& texCoord,
           const glm::vec3& normal  = glm::vec3(0.0f, 0.0f, 0.0f),
           const glm::vec3& tangent = glm::vec3(1.0f, 0.0f, 0.0f))
    {
        this->pos      = pos;
        this->texCoord = texCoord;
        this->normal   = normal;
        this->tangent  = tangent;
    }

    const glm::vec3& GetPos()      const { return pos; }
    const glm::vec2& GetTexCoord() const { return texCoord; }
    const glm::vec3& GetNormal()   const { return normal; }
    const glm::vec3& GetTangent()  const { return tangent; }

private:
    glm::vec3 pos;
    glm::vec2 texCoord;
    glm::vec3 normal;
    glm::vec3 tangent;
};

// Manages the GPU buffers that hold this mesh's geometry.
// Supports loading from an OBJ file or from raw vertex/index arrays.
// Copy and assignment are disabled because the GPU resources are owned by this object.
class Mesh
{
public:
    Mesh();
    ~Mesh();
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    // Draws the mesh.
    void draw();

    // Uploads the given vertex and index arrays to the GPU.
    void init(Vertex* vertices, unsigned int numVertices, unsigned int* indices, unsigned int numIndices);

    // Parses an OBJ file, loads its texture if an MTL file specifies one, and uploads to the GPU.
    void loadModel(const std::string& filename);

    // Uploads an already-parsed IndexedModel to the GPU.
    void initModel(const IndexedModel& model);

    // Returns true if the mesh loaded a diffuse texture from its OBJ/MTL file.
    bool hasTexture() const { return _hasTexture; }

    // Activates the mesh's own texture on the given texture slot.
    void bindTexture(unsigned int unit) { _texture.Bind(unit); }

private:
    // Which buffer holds which type of vertex data
    enum
    {
        POSITION_VERTEXBUFFER,
        TEXCOORD_VB,
        NORMAL_VB,
        TANGENT_VB,
        INDEX_VB,
        NUM_BUFFERS
    };

    GLuint       vertexArrayObject;
    GLuint       vertexArrayBuffers[NUM_BUFFERS];
    unsigned int drawCount;   // how many vertices to draw
    Texture      _texture;
    bool         _hasTexture;
};
