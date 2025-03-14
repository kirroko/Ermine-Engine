/* Start Header ************************************************************************/
/*!
\file       Camera.cpp
\author     Wong Jun Yu, Kean, keanwng\@gmail.com
\date       10/03/2025
\brief      This reflects the brief
Copyright (C) 2025 TwoJumpingRabbits
*/
/* End Header **************************************************************************/
#include "PreCompile.h"
#include "Camera.h"

#include "Components.h"
#include "ECS.h"
#include "MathUtils.h"

using namespace Ermine::graphics;

void Camera::Update(GLFWwindow* windowContext)
{
    for (auto& entity : m_Entities)
    {
        if (!ECS::GetInstance().IsEntityValid(activeCameraEntity)) // Find the active camera if we don't have one
        {
            auto& camera = ECS::GetInstance().GetComponent<CameraComponent>(entity);
            if (camera.isPrimary)
            {
                activeCameraEntity = entity;
                return;
            }
        }

        auto& camera = ECS::GetInstance().GetComponent<CameraComponent>(entity);
        auto& transform = ECS::GetInstance().GetComponent<Transform>(entity);

        // Update projection matrix when window resizes
        int width, height;
        glfwGetFramebufferSize(windowContext, &width, &height);
        camera.aspectRatio = static_cast<float>(width) / static_cast<float>(height);

        Mtx44Perspective(projectionMatrix, camera.fov, camera.aspectRatio, camera.nearPlane, camera.farPlane);

        // Calculate view matrix from transform
        Vector3D forward = CalculateForwardVector(transform.rotation);
        Vector3D up = CalculateUpVector(transform.rotation);

        Mtx44LookAt(viewMatrix,transform.position, transform.position + forward, up);
    }
}

Ermine::Vector3D Camera::CalculateUpVector(const Vector3D& rotation) const
{
    // Convert degrees to radians
    float yawRad = radian(rotation.y);
    float pitchRad = radian(rotation.x);
    float rollRad = radian(rotation.z);

    // Calculate the up vector using the rotation matrix elements for the up direction
    return Vector3D(
        -sinf(rollRad) * cosf(yawRad) - cosf(rollRad) * sinf(pitchRad) * sinf(yawRad),
        cosf(rollRad) * cosf(pitchRad),
        -sinf(rollRad) * sinf(yawRad) + cosf(rollRad) * sinf(pitchRad) * cosf(yawRad));
}

Ermine::Vector3D Camera::CalculateForwardVector(const Vector3D& rotation) const
{
    // Convert degree to radian and calculate the forward vector
    float yawRad = radian(rotation.y);
    float pitchRad = radian(rotation.x);

    return Vector3D(
        cosf(pitchRad) * sinf(yawRad),
        -sinf(pitchRad),
        cosf(pitchRad) * cosf(yawRad));
}
