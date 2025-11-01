/* Start Header ************************************************************************/
/*!
\file       GameCamera.cpp
\author     Jeremy Lim Ting Jie, jeremytingjie.lim, 2301370, jeremytingjie.lim\@digipen.edu
\date       11/03/2025
\brief      First-person game camera implementation

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "PreCompile.h"
#include "GameCamera.h"
#include "ECS.h"
#include "Components.h"
#include "Input.h"
#include <glm/ext/matrix_transform.hpp>

namespace Ermine::graphics
{
    // Private constructor for singleton
    GameCamera::GameCamera(float fov, float aspectRatio, float nearClip, float farClip)
        : m_FOV(fov), m_AspectRatio(aspectRatio), m_NearClip(nearClip), m_FarClip(farClip)
    {
        UpdateProjectionMatrix();
        UpdateViewMatrix();
    }

    void GameCamera::Update()
    {
        if (!HasValidCamera())
            return;

        // Update from entity transform
        UpdateFromEntity();

        // Process mouse look
        auto [deltaX, deltaY] = Input::GetMouseDeltaGame();
        ProcessMouseMovement(deltaX, deltaY);

        // Update view matrix
        UpdateViewMatrix();
    }

    void GameCamera::SetCameraEntity(EntityID entity)
    {
        m_CameraEntity = entity;
        if (entity != 0)
        {
            UpdateFromEntity();
            
            // Initialize yaw and pitch from default values
            InitializeOrientationFromEntity();
        }
    }

    bool GameCamera::HasValidCamera() const
    {
        if (m_CameraEntity == 0)
            return false;

        auto& ecs = ECS::GetInstance();
        return ecs.IsEntityValid(m_CameraEntity) &&
            ecs.HasComponent<Transform>(m_CameraEntity) &&
            ecs.HasComponent<CameraComponent>(m_CameraEntity);
    }

    void GameCamera::SetPerspective(float fov, float aspectRatio, float nearClip, float farClip)
    {
        m_FOV = fov;
        m_AspectRatio = aspectRatio;
        m_NearClip = nearClip;
        m_FarClip = farClip;
        UpdateProjectionMatrix();
    }

    void GameCamera::SetViewportSize(float width, float height)
    {
        if (height > 0.0f)
        {
            m_AspectRatio = width / height;
            UpdateProjectionMatrix();
        }
    }

    void GameCamera::ProcessMouseMovement(float deltaX, float deltaY)
    {
        if (!HasValidCamera())
            return;

        auto& ecs = ECS::GetInstance();
        auto& camComp = ecs.GetComponent<CameraComponent>(m_CameraEntity);

        // Apply mouse sensitivity
        deltaX *= camComp.mouseSensitivity;
        deltaY *= camComp.mouseSensitivity;

        // Apply mouse delta to yaw and pitch
        m_Yaw += deltaX;
        m_Pitch += deltaY;  // GetMouseDeltaGame() already handles Y inversion correctly

        // Clamp pitch to avoid gimbal lock
        if (m_Pitch > 89.0f)
            m_Pitch = 89.0f;
        if (m_Pitch < -89.0f)
            m_Pitch = -89.0f;

        // Calculate new front vector 
        Vector3D front;
        front.x = cos(DegToRad(m_Yaw)) * cos(DegToRad(m_Pitch));
        front.y = sin(DegToRad(m_Pitch));
        front.z = sin(DegToRad(m_Yaw)) * cos(DegToRad(m_Pitch));

        // Normalize and calculate camera basis vectors 
        Vector3D normalized_front;
        Vec3Normalize(normalized_front, front);
        m_Front = normalized_front;

        Vector3D right;
        Vec3CrossProduct(right, m_Front, m_WorldUp);
        Vec3Normalize(right, right);
        m_Right = right;

        Vector3D up;
        Vec3CrossProduct(up, m_Right, m_Front);
        Vec3Normalize(up, up);
        m_Up = up;
    }

    void GameCamera::UpdateViewMatrix()
    {
        glm::mat4 tempLookAtMatrix = glm::lookAt(
            glm::vec3(m_Position.x, m_Position.y, m_Position.z),
            glm::vec3(m_Position.x + m_Front.x, m_Position.y + m_Front.y, m_Position.z + m_Front.z),
            glm::vec3(m_Up.x, m_Up.y, m_Up.z)
        );
        m_ViewMatrix = Mtx44(&tempLookAtMatrix[0][0]);
    }

    void GameCamera::UpdateProjectionMatrix()
    {
        Mtx44Perspective(m_ProjectionMatrix, DegToRad(m_FOV), m_AspectRatio, m_NearClip, m_FarClip);
    }

    void GameCamera::UpdateFromEntity()
    {
        if (!HasValidCamera())
            return;

        auto& ecs = ECS::GetInstance();
        auto& transform = ecs.GetComponent<Transform>(m_CameraEntity);

        // Update camera position from entity transform (with eye height offset)
        m_Position = transform.position + m_CameraOffset;
    }

    void GameCamera::InitializeOrientationFromEntity()
    {
        if (!HasValidCamera())
            return;
        
        // Recalculate camera vectors using default orientation
        Vector3D front;
        front.x = cos(DegToRad(m_Yaw)) * cos(DegToRad(m_Pitch));
        front.y = sin(DegToRad(m_Pitch));
        front.z = sin(DegToRad(m_Yaw)) * cos(DegToRad(m_Pitch));

        Vector3D normalized_front;
        Vec3Normalize(normalized_front, front);
        m_Front = normalized_front;

        Vector3D right;
        Vec3CrossProduct(right, m_Front, m_WorldUp);
        Vec3Normalize(right, right);
        m_Right = right;

        Vector3D up;
        Vec3CrossProduct(up, m_Right, m_Front);
        Vec3Normalize(up, up);
        m_Up = up;
    }
}
