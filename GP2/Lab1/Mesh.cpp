// Sends vertex, texture coordinate, normal, and index data to the GPU
// and draws the mesh each frame.

#include "Mesh.h"
#include <vector>

// Initialises the mesh to an empty, undrawable state.
Mesh::Mesh()
{
    drawCount   = 0;
    _hasTexture = false;
}

// Frees all GPU buffers for this mesh.
Mesh::~Mesh()
{
    glDeleteBuffers(NUM_BUFFERS, vertexArrayBuffers);
    glDeleteVertexArrays(1, &vertexArrayObject);
}

// Converts the raw vertex and index arrays into an IndexedModel and uploads it to the GPU.
void Mesh::init(Vertex* vertices, unsigned int numVertices, unsigned int* indices, unsigned int numIndices)
{
    IndexedModel model;

    for (unsigned int i = 0; i < numVertices; i++)
    {
        model.positions.push_back(vertices[i].GetPos());
        model.texCoords.push_back(vertices[i].GetTexCoord());
        model.normals.push_back(vertices[i].GetNormal());
        model.tangents.push_back(vertices[i].GetTangent());
    }

    for (unsigned int i = 0; i < numIndices; i++)
        model.indices.push_back(indices[i]);

    initModel(model);
}

// Parses the OBJ file at filename, loads the diffuse texture if the MTL references one,
// then uploads the resulting model to the GPU.
void Mesh::loadModel(const std::string& filename)
{
    IndexedModel model = OBJModel(filename).ToIndexedModel();

    if (!model.diffuseTexturePath.empty())
    {
        _texture.init(model.diffuseTexturePath);
        _hasTexture = true;
    }

    initModel(model);
}

// Creates the GPU buffers, uploads all vertex data, and records how many triangles there are.
void Mesh::initModel(const IndexedModel& model)
{
    drawCount = model.indices.size();

    glGenVertexArrays(1, &vertexArrayObject);
    glBindVertexArray(vertexArrayObject);

    glGenBuffers(NUM_BUFFERS, vertexArrayBuffers);

    // GL_STATIC_DRAW tells the GPU this data won't change, so it can put it somewhere fast.
    glBindBuffer(GL_ARRAY_BUFFER, vertexArrayBuffers[POSITION_VERTEXBUFFER]);
    glBufferData(GL_ARRAY_BUFFER, model.positions.size() * sizeof(model.positions[0]), &model.positions[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, vertexArrayBuffers[TEXCOORD_VB]);
    glBufferData(GL_ARRAY_BUFFER, model.texCoords.size() * sizeof(model.texCoords[0]), &model.texCoords[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

    // Some OBJ files omit normals; skip the upload rather than send an empty buffer
    if (!model.normals.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, vertexArrayBuffers[NORMAL_VB]);
        glBufferData(GL_ARRAY_BUFFER, model.normals.size() * sizeof(model.normals[0]), &model.normals[0], GL_STATIC_DRAW);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
    }

    if (!model.tangents.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, vertexArrayBuffers[TANGENT_VB]);
        glBufferData(GL_ARRAY_BUFFER, model.tangents.size() * sizeof(model.tangents[0]), &model.tangents[0], GL_STATIC_DRAW);
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);
    }

    // The index buffer is part of the VAO so it's always available when drawing this mesh.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexArrayBuffers[INDEX_VB]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, model.indices.size() * sizeof(model.indices[0]), &model.indices[0], GL_STATIC_DRAW);

    // Unbind so nothing else accidentally changes this mesh's setup.
    glBindVertexArray(0);
}

// Binds the mesh and draws all its triangles.
void Mesh::draw()
{
    glBindVertexArray(vertexArrayObject);
    glDrawElements(GL_TRIANGLES, drawCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
