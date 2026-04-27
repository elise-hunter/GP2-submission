#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

// Represents the viewpoint the scene is rendered from.
// Holds a position, a look direction, and a perspective projection matrix.
struct Camera
{
public:
    Camera() {}

    // Sets up the camera with a starting position, field of view, aspect ratio,
    // and near/far draw distances. Must be called before using the camera.
    void initCamera(const glm::vec3& pos, float fov, float aspect, float nearClip, float farClip)
    {
        this->pos        = pos;
        this->forward    = glm::vec3(0.0f, 0.0f, 1.0f);
        this->up         = glm::vec3(0.0f, 1.0f, 0.0f);
        this->projection = glm::perspective(fov, aspect, nearClip, farClip);
    }

    // Returns the camera's current world position.
    glm::vec3 getPos() const
    {
        return this->pos;
    }

    // Returns the matrix that converts world positions into where they appear on screen.
    glm::mat4 getViewProjection() const
    {
        return projection * glm::lookAt(pos, pos + forward, up);
    }

    // Moves the camera forward along its look direction.
    void MoveForward(float amt)
    {
        pos += forward * amt;
    }

    // Moves the camera backward along its look direction.
    void MoveBack(float amt)
    {
        pos += -forward * amt;
    }

    // Moves the camera downward.
    void MoveDown(float amt)
    {
        pos -= up * amt;
    }

    // Moves the camera upward.
    void MoveUp(float amt)
    {
        pos += up * amt;
    }

    // Moves the camera to the right relative to its current look direction.
    void MoveRight(float amt)
    {
        pos += glm::cross(forward, up) * amt;
    }

    // Moves the camera to the left relative to its current look direction.
    void MoveLeft(float amt)
    {
        pos -= glm::cross(forward, up) * amt;
    }

    // Aims the camera at a point in the world.
    void setLook(glm::vec3 targetPos)
    {
        forward = glm::normalize(targetPos - pos);
    }

    // Teleports the camera to a new world position without changing its look direction.
    void setPos(glm::vec3 newPos)
    {
        pos = newPos;
    }

private:
    glm::mat4 projection;
    glm::vec3 pos;
    glm::vec3 forward;
    glm::vec3 up;
};
