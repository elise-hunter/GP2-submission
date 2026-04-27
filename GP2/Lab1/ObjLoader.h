#ifndef OBJLOADER_H_INCLUDED
#define OBJLOADER_H_INCLUDED

#include <glm/glm.hpp>
#include <vector>
#include <string>

// A single index entry from an OBJ face line.
// OBJ files store separate index arrays for positions, UVs, and normals,
// so each corner of a triangle needs all three indices.
struct OBJIndex
{
    unsigned int vertexIndex;
    unsigned int uvIndex;
    unsigned int normalIndex;

    // Allows OBJIndex to be used as a map key; comparison is by vertex index only.
    bool operator<(const OBJIndex& r) const { return vertexIndex < r.vertexIndex; }
};

// A fully processed mesh ready to be uploaded to the GPU.
// All arrays (positions, texCoords, normals, tangents) are the same length
// and share the same index list.
class IndexedModel
{
public:
    std::vector<glm::vec3>   positions;
    std::vector<glm::vec2>   texCoords;
    std::vector<glm::vec3>   normals;
    std::vector<glm::vec3>   tangents;
    std::vector<unsigned int> indices;

    // Resolved path to the first diffuse texture found in the MTL file; empty if none.
    std::string diffuseTexturePath;

    // Calculates smooth per-vertex normals by averaging the face normals of all
    // triangles that share each vertex.
    void CalcNormals();

    // Calculates per-vertex tangent vectors, which are needed for normal mapping.
    // Must be called after CalcNormals and only when the model has UV coordinates.
    void CalcTangents();
};

// Parses a single OBJ file and converts it into an IndexedModel.
// Also reads the referenced MTL file (if any) to find a diffuse texture path.
class OBJModel
{
public:
    std::vector<OBJIndex>  OBJIndices;
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;
    bool hasUVs;
    bool hasNormals;

    // Loads and parses the OBJ file at fileName.
    // Also parses any MTL file referenced in the OBJ to find the diffuse texture.
    OBJModel(const std::string& fileName);

    // Converts the raw OBJ data into a GPU-ready IndexedModel.
    IndexedModel ToIndexedModel();

private:
    // Resolved path to the first map_Kd texture in the MTL file; empty if none.
    std::string mtlTexturePath;

    // Looks through the sorted index list for a vertex that matches currentIndex
    // (same position, UV, and normal). Returns its index in result, or (unsigned int)-1 if not found.
    unsigned int FindLastVertexIndex(const std::vector<OBJIndex*>& indexLookup,
                                     const OBJIndex* currentIndex,
                                     const IndexedModel& result);

    // Parses a face line (e.g. "f 1/1/1 2/2/2 3/3/3") and appends the triangle's indices.
    // Handles quads by splitting them into two triangles.
    void CreateOBJFace(const std::string& line);

    // Opens and reads the given MTL file to find the first diffuse texture (map_Kd) path.
    void ParseMTL(const std::string& dir, const std::string& mtlFile);

    // Parses two space-separated floats from a "vt" UV coordinate line.
    glm::vec2 ParseOBJVec2(const std::string& line);

    // Parses three space-separated floats from a "v" or "vn" line.
    glm::vec3 ParseOBJVec3(const std::string& line);

    // Parses one vertex/uv/normal index triplet from a face token (e.g. "4/2/1").
    // Sets hasUVs and hasNormals to true if the token contains those components.
    OBJIndex ParseOBJIndex(const std::string& token, bool* hasUVs, bool* hasNormals);
};

#endif // OBJLOADER_H_INCLUDED
