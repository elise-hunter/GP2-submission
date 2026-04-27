#pragma once
#include <string>
#include <GL\glew.h>
#include "Transform.h"
#include "Camera.h"
#include <iostream>
#include <fstream>

// Compiles shader source files and links them into a program ready to run on the GPU.
// Has helpers for passing values to the shader (floats, vectors, matrices, etc.).
// Copy and assignment are disabled because the GPU program is owned by this object.
class Shader
{
public:
    Shader();
    ~Shader();
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    // Makes this shader the active one so the next draw calls use it.
    void Bind();

    // Sends the object's position/transform to the shader so it knows where to draw it.
    void Update(const Transform& transform, const Camera& camera);

    // Compiles vertex and fragment shaders from files and links them into a program.
    void init(const std::string& vertFile, const std::string& fragFile);

    // Compiles vertex, geometry, and fragment shaders from files and links them into a program.
    void initGeo(const std::string& vertFile, const std::string& geomFile, const std::string& fragFile);

    // Reads shader source from a file and returns it as a string.
    std::string Shader::LoadShader(const std::string& fileName);

    // Checks whether a shader or program compiled/linked successfully, and prints any errors.
    void Shader::CheckShaderError(GLuint shader, GLuint flag, bool isProgram, const std::string& errorMessage);

    // Compiles a single shader stage from source text and returns its ID.
    GLuint Shader::CreateShader(const std::string& text, unsigned int type);

    // Returns the OpenGL program object ID.
    GLuint ID() { return shaderID; }

    // Sets a true/false value in the shader (sent as 0 or 1).
    void setBool(const std::string& name, bool value) const
    {
        GLint loc = glGetUniformLocation(shaderID, name.c_str());
        if (loc == -1) std::cerr << "Unable to find uniform: " << name << std::endl;
        glUniform1i(loc, (int)value);
    }

    // Sets an integer value in the shader.
    void setInt(const std::string& name, int value) const
    {
        GLint loc = glGetUniformLocation(shaderID, name.c_str());
        if (loc == -1) std::cerr << "Unable to find uniform: " << name << std::endl;
        glUniform1i(loc, value);
    }

    // Sets a float value in the shader.
    void setFloat(const std::string& name, float value) const
    {
        GLint loc = glGetUniformLocation(shaderID, name.c_str());
        glUniform1f(loc, value);
    }

    // Sets a two-component float value in the shader.
    void setVec2(const std::string& name, const glm::vec2& value) const
    {
        GLint loc = glGetUniformLocation(shaderID, name.c_str());
        if (loc == -1) std::cerr << "Unable to find uniform: " << name << std::endl;
        glUniform2fv(loc, 1, &value[0]);
    }

    // Sets a two-component float value in the shader from two separate floats.
    void setVec2(const std::string& name, float x, float y) const
    {
        GLint loc = glGetUniformLocation(shaderID, name.c_str());
        if (loc == -1) std::cerr << "Unable to find uniform: " << name << std::endl;
        glUniform2f(loc, x, y);
    }

    // Sets a three-component float value in the shader.
    void setVec3(const std::string& name, const glm::vec3& value) const
    {
        GLint loc = glGetUniformLocation(shaderID, name.c_str());
        if (loc == -1) std::cerr << "Unable to find uniform: " << name << std::endl;
        glUniform3fv(loc, 1, &value[0]);
    }

    // Sets a three-component float value in the shader from three separate floats.
    void setVec3(const std::string& name, float x, float y, float z) const
    {
        GLint loc = glGetUniformLocation(shaderID, name.c_str());
        if (loc == -1) std::cerr << "Unable to find uniform: " << name << std::endl;
        glUniform3f(loc, x, y, z);
    }

    // Sets a four-component float value in the shader.
    void setVec4(const std::string& name, const glm::vec4& value) const
    {
        GLint loc = glGetUniformLocation(shaderID, name.c_str());
        if (loc == -1) std::cerr << "Unable to find uniform: " << name << std::endl;
        glUniform4fv(loc, 1, &value[0]);
    }

    // Sets a four-component float value in the shader from four separate floats.
    void setVec4(const std::string& name, float x, float y, float z, float w)
    {
        GLint loc = glGetUniformLocation(shaderID, name.c_str());
        if (loc == -1) std::cerr << "Unable to find uniform: " << name << std::endl;
        glUniform4f(loc, x, y, z, w);
    }

    // Sets a 4x4 matrix value in the shader.
    void setMat4(const std::string& name, const glm::mat4& mat) const
    {
        GLint loc = glGetUniformLocation(shaderID, name.c_str());
        if (loc == -1) std::cerr << "Unable to find uniform: " << name << std::endl;
        glUniformMatrix4fv(loc, 1, GL_FALSE, &mat[0][0]);
    }

private:
    // Up to three shader stages: vertex, optional geometry, and fragment
    static const unsigned int MAX_SHADERS = 3;

    enum
    {
        TRANSFORM_U,   // cached location of the transform matrix in the shader
        NUM_UNIFORMS
    };

    GLuint shaderID;
    GLuint shaders[MAX_SHADERS];
    int    m_shaderCount;
    GLuint uniforms[NUM_UNIFORMS];
};
