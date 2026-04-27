#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

// Stores the position, rotation, and scale of a single object in the world.
// Call GetModel() to get the 4x4 matrix that transforms the object's local space into world space.
struct Transform
{
public:
    // Initialises with an optional position, rotation (Euler angles in radians), and scale.
    Transform(
        const glm::vec3& pos   = glm::vec3(0.0f),
        const glm::vec3& rot   = glm::vec3(0.0f),
        const glm::vec3& scale = glm::vec3(1.0f))
    {
        this->pos   = pos;
        this->rot   = rot;
        this->scale = scale;
    }

    // Builds and returns the model matrix by combining translation, rotation, and scale.
    // Rotation is applied as X * Y * Z Euler angles.
    glm::mat4 GetModel() const
    {
        glm::mat4 posMat   = glm::translate(pos);
        glm::mat4 scaleMat = glm::scale(scale);
        glm::mat4 rotX     = glm::rotate(rot.x, glm::vec3(1.0f, 0.0f, 0.0f));
        glm::mat4 rotY     = glm::rotate(rot.y, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 rotZ     = glm::rotate(rot.z, glm::vec3(0.0f, 0.0f, 1.0f));
        glm::mat4 rotMat   = rotX * rotY * rotZ;

        return posMat * rotMat * scaleMat;
    }

    void SetPos(const glm::vec3& pos)     { this->pos   = pos;   }
    void SetRot(const glm::vec3& rot)     { this->rot   = rot;   }
    void SetScale(const glm::vec3& scale) { this->scale = scale; }

private:
    glm::vec3 pos;
    glm::vec3 rot;
    glm::vec3 scale;
};
