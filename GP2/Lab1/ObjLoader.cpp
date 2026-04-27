// Parses Wavefront OBJ files and converts them into IndexedModels ready for GPU upload.
// Also reads MTL files to find diffuse texture paths.

#include "ObjLoader.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <map>

// Forward declarations for file-local helper functions
static bool CompareOBJIndexPtr(const OBJIndex* a, const OBJIndex* b);
static inline unsigned int FindNextChar(unsigned int start, const char* str, unsigned int length, char token);
static inline unsigned int ParseOBJIndexValue(const std::string& token, unsigned int start, unsigned int end);
static inline float        ParseOBJFloatValue(const std::string& token, unsigned int start, unsigned int end);
static inline std::vector<std::string> SplitString(const std::string& s, char delim);
static inline std::string  TrimLine(const std::string& s);
static inline std::string  GetDirectory(const std::string& filepath);

// Reads the OBJ file at fileName line by line, parsing vertices, UVs, normals, and faces.
// If an mtllib directive is found, the referenced MTL file is parsed afterwards.
OBJModel::OBJModel(const std::string& fileName)
{
    hasUVs    = false;
    hasNormals = false;

    std::string directory  = GetDirectory(fileName);
    std::string pendingMtl;

    std::ifstream file(fileName.c_str());
    if (!file.is_open())
    {
        std::cerr << "Unable to load mesh: " << fileName << std::endl;
        return;
    }

    std::string line;
    while (file.good())
    {
        getline(file, line);

        if (line.length() < 2) continue;

        const char* lineCStr = line.c_str();

        switch (lineCStr[0])
        {
        case 'v':
            if (lineCStr[1] == 't')
            {
                glm::vec2 uv = ParseOBJVec2(line);
                uv.y = 1.0f - uv.y;  // OBJ V=0 is top; OpenGL V=0 is bottom
                this->uvs.push_back(uv);
            }
            else if (lineCStr[1] == 'n')
                this->normals.push_back(ParseOBJVec3(line));
            else if (lineCStr[1] == ' ' || lineCStr[1] == '\t')
                this->vertices.push_back(ParseOBJVec3(line));
            break;

        case 'f':
            CreateOBJFace(line);
            break;

        case 'm':
            // "mtllib <filename>": save the name to parse after all geometry is read
            if (line.length() > 7 && line.substr(0, 6) == "mtllib")
                pendingMtl = TrimLine(line.substr(7));
            break;

        default:
            break;
        }
    }

    if (!pendingMtl.empty())
        ParseMTL(directory, pendingMtl);
}

// Calculates smooth per-vertex normals by accumulating the face normal of every
// triangle that touches each vertex, then normalizing the result.
void IndexedModel::CalcNormals()
{
    for (unsigned int i = 0; i < indices.size(); i += 3)
    {
        int i0 = indices[i];
        int i1 = indices[i + 1];
        int i2 = indices[i + 2];

        glm::vec3 edge1  = positions[i1] - positions[i0];
        glm::vec3 edge2  = positions[i2] - positions[i0];
        glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

        normals[i0] += normal;
        normals[i1] += normal;
        normals[i2] += normal;
    }

    for (unsigned int i = 0; i < positions.size(); i++)
        normals[i] = glm::normalize(normals[i]);
}

// Calculates per-vertex tangent vectors using the UV deltas of each triangle.
// Tangents point in the direction of increasing U in texture space, which is
// used by the normal-mapping shader to transform sampled normals into world space.
void IndexedModel::CalcTangents()
{
    tangents.resize(positions.size(), glm::vec3(0.0f));

    for (unsigned int i = 0; i < indices.size(); i += 3)
    {
        int i0 = indices[i];
        int i1 = indices[i + 1];
        int i2 = indices[i + 2];

        glm::vec3 edge1 = positions[i1] - positions[i0];
        glm::vec3 edge2 = positions[i2] - positions[i0];

        float deltaU1 = texCoords[i1].x - texCoords[i0].x;
        float deltaV1 = texCoords[i1].y - texCoords[i0].y;
        float deltaU2 = texCoords[i2].x - texCoords[i0].x;
        float deltaV2 = texCoords[i2].y - texCoords[i0].y;

        // The determinant of the UV matrix; guarded against division by zero
        float denom  = deltaU1 * deltaV2 - deltaU2 * deltaV1;
        float factor = (fabsf(denom) > 0.00001f) ? 1.0f / denom : 0.0f;

        glm::vec3 tangent;
        tangent.x = factor * (deltaV2 * edge1.x - deltaV1 * edge2.x);
        tangent.y = factor * (deltaV2 * edge1.y - deltaV1 * edge2.y);
        tangent.z = factor * (deltaV2 * edge1.z - deltaV1 * edge2.z);

        // Accumulate: each vertex gets contributions from all triangles that share it
        tangents[i0] += tangent;
        tangents[i1] += tangent;
        tangents[i2] += tangent;
    }

    for (unsigned int i = 0; i < tangents.size(); i++)
        if (glm::length(tangents[i]) > 0.0f)
            tangents[i] = glm::normalize(tangents[i]);
}

// Converts the raw OBJ data into an IndexedModel.
// Two intermediate models are built in parallel:
//   - normalModel: one entry per unique OBJIndex, used to calculate smooth normals
//   - result:      one entry per unique (position + UV + normal) combination, used for rendering
// After building both, the calculated normals are copied from normalModel into result.
IndexedModel OBJModel::ToIndexedModel()
{
    IndexedModel result;
    IndexedModel normalModel;

    unsigned int numIndices = OBJIndices.size();

    // Build a sorted copy of the index pointers so we can binary-search by vertex index
    std::vector<OBJIndex*> indexLookup;
    for (unsigned int i = 0; i < numIndices; i++)
        indexLookup.push_back(&OBJIndices[i]);
    std::sort(indexLookup.begin(), indexLookup.end(), CompareOBJIndexPtr);

    // Maps OBJIndex -> position in normalModel (for normal smoothing)
    std::map<OBJIndex, unsigned int> normalModelIndexMap;
    // Maps result index -> normalModel index (for copying normals back at the end)
    std::map<unsigned int, unsigned int> indexMap;

    for (unsigned int i = 0; i < numIndices; i++)
    {
        OBJIndex* currentIndex = &OBJIndices[i];

        glm::vec3 currentPosition = vertices[currentIndex->vertexIndex];
        glm::vec2 currentTexCoord = hasUVs    ? uvs[currentIndex->uvIndex]         : glm::vec2(0.0f);
        glm::vec3 currentNormal   = hasNormals ? normals[currentIndex->normalIndex] : glm::vec3(0.0f);

        // Add to normalModel if this exact OBJIndex hasn't been seen before
        unsigned int normalModelIndex;
        auto normalIt = normalModelIndexMap.find(*currentIndex);
        if (normalIt == normalModelIndexMap.end())
        {
            normalModelIndex = normalModel.positions.size();
            normalModelIndexMap.insert({ *currentIndex, normalModelIndex });
            normalModel.positions.push_back(currentPosition);
            normalModel.texCoords.push_back(currentTexCoord);
            normalModel.normals.push_back(currentNormal);
        }
        else
        {
            normalModelIndex = normalIt->second;
        }

        // Add to result if no existing vertex matches (position + UV + normal must all match)
        unsigned int resultModelIndex;
        unsigned int previousLocation = FindLastVertexIndex(indexLookup, currentIndex, result);
        if (previousLocation == (unsigned int)-1)
        {
            resultModelIndex = result.positions.size();
            result.positions.push_back(currentPosition);
            result.texCoords.push_back(currentTexCoord);
            result.normals.push_back(currentNormal);
        }
        else
        {
            resultModelIndex = previousLocation;
        }

        normalModel.indices.push_back(normalModelIndex);
        result.indices.push_back(resultModelIndex);
        indexMap.insert({ resultModelIndex, normalModelIndex });
    }

    // If the OBJ had no normals, calculate them now and copy into result
    if (!hasNormals)
    {
        normalModel.CalcNormals();
        for (unsigned int i = 0; i < result.positions.size(); i++)
            result.normals[i] = normalModel.normals[indexMap[i]];
    }

    result.diffuseTexturePath = mtlTexturePath;

    if (hasUVs)
        result.CalcTangents();

    return result;
}

// Searches the sorted indexLookup for any existing vertex in result that matches
// currentIndex exactly (same position, UV, and normal).
// Returns the matching index in result.positions, or (unsigned int)-1 if not found.
unsigned int OBJModel::FindLastVertexIndex(const std::vector<OBJIndex*>& indexLookup,
                                            const OBJIndex* currentIndex,
                                            const IndexedModel& result)
{
    unsigned int start   = 0;
    unsigned int end     = indexLookup.size();
    unsigned int current = (end - start) / 2 + start;
    unsigned int previous = start;

    while (current != previous)
    {
        OBJIndex* testIndex = indexLookup[current];

        if (testIndex->vertexIndex == currentIndex->vertexIndex)
        {
            // Found the vertex index; now scan nearby entries for one that also
            // matches UV and normal indices
            unsigned int countStart = current;

            for (unsigned int i = 0; i < current; i++)
            {
                OBJIndex* possibleIndex = indexLookup[current - i];
                if (possibleIndex == currentIndex) continue;
                if (possibleIndex->vertexIndex != currentIndex->vertexIndex) break;
                countStart--;
            }

            for (unsigned int i = countStart; i < indexLookup.size() - countStart; i++)
            {
                OBJIndex* possibleIndex = indexLookup[current + i];
                if (possibleIndex == currentIndex) continue;
                if (possibleIndex->vertexIndex != currentIndex->vertexIndex) break;

                bool uvMatch     = !hasUVs    || possibleIndex->uvIndex     == currentIndex->uvIndex;
                bool normalMatch = !hasNormals || possibleIndex->normalIndex == currentIndex->normalIndex;

                if (uvMatch && normalMatch)
                {
                    glm::vec3 pos      = vertices[currentIndex->vertexIndex];
                    glm::vec2 texCoord = hasUVs    ? uvs[currentIndex->uvIndex]         : glm::vec2(0.0f);
                    glm::vec3 normal   = hasNormals ? normals[currentIndex->normalIndex] : glm::vec3(0.0f);

                    for (unsigned int j = 0; j < result.positions.size(); j++)
                    {
                        if (pos == result.positions[j]
                            && (!hasUVs    || texCoord == result.texCoords[j])
                            && (!hasNormals || normal   == result.normals[j]))
                        {
                            return j;
                        }
                    }
                }
            }

            return (unsigned int)-1;
        }
        else
        {
            if (testIndex->vertexIndex < currentIndex->vertexIndex)
                start = current;
            else
                end = current;
        }

        previous = current;
        current  = (end - start) / 2 + start;
    }

    return (unsigned int)-1;
}

// Parses a face line (e.g. "f 1/2/3 4/5/6 7/8/9") and appends the triangle's indices.
// If the face is a quad (4 vertices), it is split into two triangles.
void OBJModel::CreateOBJFace(const std::string& line)
{
    std::vector<std::string> tokens = SplitString(line, ' ');

    this->OBJIndices.push_back(ParseOBJIndex(tokens[1], &this->hasUVs, &this->hasNormals));
    this->OBJIndices.push_back(ParseOBJIndex(tokens[2], &this->hasUVs, &this->hasNormals));
    this->OBJIndices.push_back(ParseOBJIndex(tokens[3], &this->hasUVs, &this->hasNormals));

    // If there is a fourth vertex, create a second triangle (fan triangulation)
    if ((int)tokens.size() > 4)
    {
        this->OBJIndices.push_back(ParseOBJIndex(tokens[1], &this->hasUVs, &this->hasNormals));
        this->OBJIndices.push_back(ParseOBJIndex(tokens[3], &this->hasUVs, &this->hasNormals));
        this->OBJIndices.push_back(ParseOBJIndex(tokens[4], &this->hasUVs, &this->hasNormals));
    }
}

// Parses one face token (e.g. "4", "4/2", "4/2/1", or "4//1") into an OBJIndex.
// Sets *hasUVs and *hasNormals to true if the token contains those components.
OBJIndex OBJModel::ParseOBJIndex(const std::string& token, bool* hasUVs, bool* hasNormals)
{
    unsigned int tokenLength = token.length();
    const char*  tokenString = token.c_str();

    unsigned int vertIndexStart = 0;
    unsigned int vertIndexEnd   = FindNextChar(vertIndexStart, tokenString, tokenLength, '/');

    OBJIndex result;
    result.vertexIndex = ParseOBJIndexValue(token, vertIndexStart, vertIndexEnd);
    result.uvIndex     = 0;
    result.normalIndex = 0;

    if (vertIndexEnd >= tokenLength)
        return result;

    vertIndexStart = vertIndexEnd + 1;
    vertIndexEnd   = FindNextChar(vertIndexStart, tokenString, tokenLength, '/');

    result.uvIndex = ParseOBJIndexValue(token, vertIndexStart, vertIndexEnd);
    *hasUVs = true;

    if (vertIndexEnd >= tokenLength)
        return result;

    vertIndexStart = vertIndexEnd + 1;
    vertIndexEnd   = FindNextChar(vertIndexStart, tokenString, tokenLength, '/');

    result.normalIndex = ParseOBJIndexValue(token, vertIndexStart, vertIndexEnd);
    *hasNormals = true;

    return result;
}

// Parses three floats from a "v" or "vn" OBJ line and returns them as a vec3.
glm::vec3 OBJModel::ParseOBJVec3(const std::string& line)
{
    unsigned int tokenLength = line.length();
    const char*  tokenString = line.c_str();

    // Skip past the line prefix ("v " or "vn ") and any extra spaces
    unsigned int start = 2;
    while (start < tokenLength && tokenString[start] == ' ')
        start++;

    unsigned int end = FindNextChar(start, tokenString, tokenLength, ' ');
    float x = ParseOBJFloatValue(line, start, end);

    start = end + 1;
    end   = FindNextChar(start, tokenString, tokenLength, ' ');
    float y = ParseOBJFloatValue(line, start, end);

    start = end + 1;
    end   = FindNextChar(start, tokenString, tokenLength, ' ');
    float z = ParseOBJFloatValue(line, start, end);

    return glm::vec3(x, y, z);
}

// Parses two floats from a "vt" UV coordinate line and returns them as a vec2.
glm::vec2 OBJModel::ParseOBJVec2(const std::string& line)
{
    unsigned int tokenLength = line.length();
    const char*  tokenString = line.c_str();

    // Skip past "vt " and any extra spaces
    unsigned int start = 3;
    while (start < tokenLength && tokenString[start] == ' ')
        start++;

    unsigned int end = FindNextChar(start, tokenString, tokenLength, ' ');
    float x = ParseOBJFloatValue(line, start, end);

    start = end + 1;
    end   = FindNextChar(start, tokenString, tokenLength, ' ');
    float y = ParseOBJFloatValue(line, start, end);

    return glm::vec2(x, y);
}

// Opens an MTL file and scans it for the first "map_Kd" entry (the diffuse texture).
// Stores the resolved path in mtlTexturePath.
void OBJModel::ParseMTL(const std::string& dir, const std::string& mtlFile)
{
    std::ifstream file(dir + mtlFile);
    if (!file.is_open())
    {
        std::cerr << "Unable to load MTL: " << dir + mtlFile << std::endl;
        return;
    }

    std::string line;
    while (file.good())
    {
        getline(file, line);
        if (line.length() > 7 && line.substr(0, 6) == "map_Kd")
        {
            mtlTexturePath = dir + TrimLine(line.substr(7));
            break;  // Only need the first diffuse texture
        }
    }
}

// Comparison function used when sorting OBJIndex pointers by vertex index.
static bool CompareOBJIndexPtr(const OBJIndex* a, const OBJIndex* b)
{
    return a->vertexIndex < b->vertexIndex;
}

// Scans forward from start in str until it finds token or reaches length.
// Returns the index of the found character (or length if not found).
static inline unsigned int FindNextChar(unsigned int start, const char* str, unsigned int length, char token)
{
    unsigned int result = start;
    while (result < length)
    {
        result++;
        if (str[result] == token)
            break;
    }
    return result;
}

// Extracts the substring from token between start and end, converts it to an integer,
// and subtracts 1 (OBJ indices are 1-based; we store them 0-based).
static inline unsigned int ParseOBJIndexValue(const std::string& token, unsigned int start, unsigned int end)
{
    return atoi(token.substr(start, end - start).c_str()) - 1;
}

// Extracts the substring from token between start and end and converts it to a float.
static inline float ParseOBJFloatValue(const std::string& token, unsigned int start, unsigned int end)
{
    return (float)atof(token.substr(start, end - start).c_str());
}

// Splits the string s on delim and returns all the resulting substrings.
static inline std::vector<std::string> SplitString(const std::string& s, char delim)
{
    std::vector<std::string> elems;

    const char*  cstr      = s.c_str();
    unsigned int strLength = s.length();
    unsigned int start     = 0;
    unsigned int end       = 0;

    while (end <= strLength)
    {
        while (end <= strLength)
        {
            if (cstr[end] == delim) break;
            end++;
        }
        elems.push_back(s.substr(start, end - start));
        start = end + 1;
        end   = start;
    }

    return elems;
}

// Strips leading/trailing whitespace and carriage returns from a string.
// Needed because OBJ files from Windows may have \r\n line endings.
static inline std::string TrimLine(const std::string& s)
{
    std::string result = s;
    while (!result.empty() && (result.front() == ' ' || result.front() == '\t'))
        result.erase(result.begin());
    while (!result.empty() && (result.back() == ' ' || result.back() == '\t'
                             || result.back() == '\r' || result.back() == '\n'))
        result.pop_back();
    return result;
}

// Returns the directory portion of a file path including the trailing slash,
// so it can be prepended to relative paths found inside the file (e.g. MTL references).
static inline std::string GetDirectory(const std::string& filepath)
{
    size_t pos = filepath.find_last_of("/\\");
    return (pos == std::string::npos) ? "" : filepath.substr(0, pos + 1);
}
