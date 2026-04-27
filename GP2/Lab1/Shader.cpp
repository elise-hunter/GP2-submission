// Loads GLSL source files, compiles them, and links them into a shader program.

#include "Shader.h"

// Size of the character buffer used to retrieve compile and link error messages
static const int ShaderErrorBufferSize = 1024;

// Zeroes the shader count; actual initialisation happens in init() or initGeo().
Shader::Shader()
{
    m_shaderCount = 0;
}

// Cleans up all the compiled shader stages and the program they were linked into.
Shader::~Shader()
{
    for (int i = 0; i < m_shaderCount; i++)
    {
        glDetachShader(shaderID, shaders[i]);
        glDeleteShader(shaders[i]);
    }
    glDeleteProgram(shaderID);
}

// Compiles a vertex and fragment shader from files and links them into a program.
void Shader::init(const std::string& vertFile, const std::string& fragFile)
{
    shaderID   = glCreateProgram();
    shaders[0] = CreateShader(LoadShader(vertFile), GL_VERTEX_SHADER);
    shaders[1] = CreateShader(LoadShader(fragFile), GL_FRAGMENT_SHADER);
    m_shaderCount = 2;

    for (int i = 0; i < m_shaderCount; i++)
        glAttachShader(shaderID, shaders[i]);

    // Bind attribute locations before linking so the layout stays consistent.
    glBindAttribLocation(shaderID, 0, "VertexPosition");
    glBindAttribLocation(shaderID, 1, "VertexTexCoord");
    glBindAttribLocation(shaderID, 2, "VertexNormal");

    glLinkProgram(shaderID);
    CheckShaderError(shaderID, GL_LINK_STATUS, true, "Error: Shader program linking failed");

    // Validate checks that the program can actually run in the current OpenGL state.
    glValidateProgram(shaderID);
    CheckShaderError(shaderID, GL_VALIDATE_STATUS, true, "Error: Shader program not valid");

    // Cache the transform matrix location so it doesn't need looking up every frame.
    uniforms[TRANSFORM_U] = glGetUniformLocation(shaderID, "transform");
}

// Compiles vertex, geometry, and fragment shaders from files and links them into a program.
void Shader::initGeo(const std::string& vertFile, const std::string& geomFile, const std::string& fragFile)
{
    shaderID   = glCreateProgram();
    shaders[0] = CreateShader(LoadShader(vertFile),  GL_VERTEX_SHADER);
    shaders[1] = CreateShader(LoadShader(geomFile),  GL_GEOMETRY_SHADER);
    shaders[2] = CreateShader(LoadShader(fragFile),  GL_FRAGMENT_SHADER);
    m_shaderCount = 3;

    for (int i = 0; i < m_shaderCount; i++)
        glAttachShader(shaderID, shaders[i]);

    glBindAttribLocation(shaderID, 0, "VertexPosition");
    glBindAttribLocation(shaderID, 1, "VertexTexCoord");
    glBindAttribLocation(shaderID, 2, "VertexNormal");

    glLinkProgram(shaderID);
    CheckShaderError(shaderID, GL_LINK_STATUS, true, "Error: Shader program linking failed");

    glValidateProgram(shaderID);
    CheckShaderError(shaderID, GL_VALIDATE_STATUS, true, "Error: Shader program not valid");

    uniforms[TRANSFORM_U] = glGetUniformLocation(shaderID, "transform");
}

// Activates this shader so the next thing drawn uses it.
void Shader::Bind()
{
    glUseProgram(shaderID);
}

// Sends the object's transform and the camera's view to the shader.
void Shader::Update(const Transform& transform, const Camera& camera)
{
    glm::mat4 model = transform.GetModel();
    glm::mat4 mvp   = camera.getViewProjection() * model;
    glUniformMatrix4fv(uniforms[TRANSFORM_U], 1, GL_FALSE, &mvp[0][0]);

    GLint modelLoc = glGetUniformLocation(shaderID, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
}

// Compiles a single shader stage from source text and returns its ID.
GLuint Shader::CreateShader(const std::string& text, unsigned int type)
{
    GLuint shader = glCreateShader(type);

    if (shader == 0)
        std::cerr << "Error: shader type creation failed: " << type << std::endl;

    // glShaderSource expects an array of strings, so wrap the source in a single-element array.
    const GLchar* stringSource[1] = { text.c_str() };
    GLint         lengths[1]      = { (GLint)text.length() };

    glShaderSource(shader, 1, stringSource, lengths);
    glCompileShader(shader);
    CheckShaderError(shader, GL_COMPILE_STATUS, false, "Error: shader compilation failed");

    return shader;
}

// Reads the entire contents of a text file and returns them as a string.
std::string Shader::LoadShader(const std::string& fileName)
{
    std::ifstream file(fileName.c_str());
    std::string output, line;

    if (file.is_open())
    {
        while (file.good())
        {
            getline(file, line);
            output.append(line + "\n");
        }
    }
    else
    {
        std::cerr << "Unable to load shader: " << fileName << std::endl;
    }

    return output;
}

// Checks whether a shader or program compiled/linked successfully, and prints the error log if not.
void Shader::CheckShaderError(GLuint shader, GLuint flag, bool isProgram, const std::string& errorMessage)
{
    GLint  success = 0;
    GLchar error[ShaderErrorBufferSize] = { 0 };

    // Shaders and programs use different query functions.
    if (isProgram)
        glGetProgramiv(shader, flag, &success);
    else
        glGetShaderiv(shader, flag, &success);

    if (success == GL_FALSE)
    {
        if (isProgram)
            glGetProgramInfoLog(shader, sizeof(error), NULL, error);
        else
            glGetShaderInfoLog(shader, sizeof(error), NULL, error);

        std::cerr << errorMessage << ": '" << error << "'" << std::endl;
    }
}
