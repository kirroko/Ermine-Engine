/* Start Header ************************************************************************/
/*!
\file       EditorCamera.h
\author     Wong Jun Yu, Kean, keanwng\@gmail.com
\date       11/03/2025
\brief      This file contains the declaration of the EditorCamera class.
            Provides camera functionality for the editor view.
Copyright (C) 2025 TwoJumpingRabbits
*/
/* End Header **************************************************************************/

#pragma once
#include "Matrix4x4.h"

namespace Ermine::editor
{
    class EditorCamera
    {
    public:
        EditorCamera(float fov = 45.0f, float aspectRatio = 16.0f/9.0f, float near = 0.1f, float far = 1000.0f);

        void Update(float deltaTime);

        // Getters for view and projection matrices
        const Mtx44& GetViewMatrix() const { return m_ViewMatrix;}
        const Mtx44& GetProjectionMatrix() const { return m_ProjectionMatrix; }

        // Camera controls
        void SetPosition(const Vector3D& position) { m_Position = position; }
        Vector3D GetPosition() const { return m_Position; }

        void SetPerspective(float fov, float aspectRatio, float near, float far);
        void SetViewportSize(float width, float height);

    private:
        void UpdateViewMatrix();
        void UpdateProjectionMatrix();
        void ProcessKeyboardInput(float deltaTime);
        void ProcessMouseMovement();
        void ProcessScrollWheel(float yOffset);
        
        // Camera attributes
        Vector3D m_Position{0.0f, 0.0f, 3.0f};
        Vector3D m_Front{0.0f, 0.0f, -1.0f};
        Vector3D m_Up{0.0f, 1.0f, 0.0f};
        Vector3D m_Right{1.0f, 0.0f, 0.0f};
        Vector3D m_WorldUp{0.0f, 1.0f, 0.0f};
    
        // Camera options
        float m_MovementSpeed = 5.0f;
        float m_MouseSensitivity = 0.1f;
    
        // Euler angles
        float m_Yaw = -90.0f;
        float m_Pitch = 0.0f;

        // Perspective parameters
        float m_FOV;
        float m_AspectRatio;
        float m_NearClip;
        float m_FarClip;
    
        // Mouse tracking
        bool m_FirstMouse = true;
        float m_LastX = 0.0f;
        float m_LastY = 0.0f;
    
        // Matrices
        Mtx44 m_ViewMatrix;
        Mtx44 m_ProjectionMatrix;
    };
}

