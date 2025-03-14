/* Start Header ************************************************************************/
/*!
\file       EditorCamera.cpp
\author     Wong Jun Yu, Kean, keanwng\@gmail.com
\date       09/03/2025
\brief      This file contains the implementation of the EditorCamera class.
Copyright (C) 2025 TwoJumpingRabbits
*/
/* End Header **************************************************************************/
#include "PreCompile.h"
#include "EditorCamera.h"

#include <algorithm>

#include "Input.h"
#include "MathUtils.h"

using namespace Ermine::editor;

EditorCamera::EditorCamera(float fov, float aspectRatio, float nearClip, float farClip)
    : m_FOV(fov), m_AspectRatio(aspectRatio), m_NearClip(nearClip), m_FarClip(farClip)
{
    UpdateViewMatrix();
    UpdateProjectionMatrix();
}

void EditorCamera::Update(float deltaTime)
{
    ProcessKeyboardInput(deltaTime);
    ProcessMouseMovement();
    UpdateViewMatrix();
}

void EditorCamera::SetPerspective(float fov, float aspectRatio, float nearClip, float farClip)
{
    m_FOV = fov;
    m_AspectRatio = aspectRatio;
    m_NearClip = nearClip;
    m_FarClip = farClip;
    UpdateProjectionMatrix();
}

void EditorCamera::SetViewportSize(float width, float height)
{
    m_AspectRatio = width / height;
    UpdateProjectionMatrix();
}

void EditorCamera::UpdateViewMatrix()
{
    // Calculate the new front vector
    Vector3D front;
    front.x = cos(radian(m_Yaw)) * cos(radian(m_Pitch));
    front.y = sin(radian(m_Pitch));
    front.z = sin(radian(m_Yaw)) * cos(radian(m_Pitch));
    
    Vector3D normalized_front;
    Vec3Normalize(normalized_front, front);
    m_Front = normalized_front;
    
    // Re-calculate the Right and Up vector
    Vector3D right;
    Vec3CrossProduct(right, m_Front, m_WorldUp);
    Vec3Normalize(right, right);
    m_Right = right;
    
    Vector3D up;
    Vec3CrossProduct(up, m_Right, m_Front);
    Vec3Normalize(up, up);
    m_Up = up;
    
    // Create the view matrix
    Mtx44LookAt(m_ViewMatrix, m_Position, m_Position + m_Front, m_Up);
}

void EditorCamera::UpdateProjectionMatrix()
{
    Mtx44Perspective(m_ProjectionMatrix, m_FOV, m_AspectRatio, m_NearClip, m_FarClip);
}

void EditorCamera::ProcessKeyboardInput(float deltaTime)
{
    float velocity = m_MovementSpeed * deltaTime;

    if (Input::IsKeyPressed(GLFW_KEY_W))
        m_Position = m_Position + m_Front * velocity;
    if (Input::IsKeyPressed(GLFW_KEY_S))
        m_Position = m_Position - m_Front * velocity;
    if (Input::IsKeyPressed(GLFW_KEY_A))
        m_Position = m_Position - m_Right * velocity;
    if (Input::IsKeyPressed(GLFW_KEY_D))
        m_Position = m_Position + m_Right * velocity;
    if (Input::IsKeyPressed(GLFW_KEY_Q))
        m_Position = m_Position - m_WorldUp * velocity;
    if (Input::IsKeyPressed(GLFW_KEY_E))
        m_Position = m_Position + m_WorldUp * velocity;
}

void EditorCamera::ProcessMouseMovement()
{
    // Only rotate camera if right mouse button is pressed
    if (Input::IsMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT))
    {
        double xpos = Input::GetMouseX(), ypos = Input::GetMouseY();
        // glfwGetCursorPos(window, &xpos, &ypos);
        
        if (m_FirstMouse)
        {
            m_LastX = static_cast<float>(xpos);
            m_LastY = static_cast<float>(ypos);
            m_FirstMouse = false;
        }
        
        float xoffset = static_cast<float>(xpos) - m_LastX;
        float yoffset = m_LastY - static_cast<float>(ypos); // Reversed: y ranges bottom to top
        
        m_LastX = static_cast<float>(xpos);
        m_LastY = static_cast<float>(ypos);
        
        xoffset *= m_MouseSensitivity;
        yoffset *= m_MouseSensitivity;
        
        m_Yaw += xoffset;
        m_Pitch += yoffset;
        
        // Constrain pitch to avoid flipping
        m_Pitch = std::min(m_Pitch, 89.0f);
        m_Pitch = std::max(m_Pitch, -89.0f);
    }
    else
    {
        m_FirstMouse = true;
    }
}
