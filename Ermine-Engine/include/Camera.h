/* Start Header ************************************************************************/
/*!
\file       Camera.h
\author     Wong Jun Yu, Kean, keanwng\@gmail.com
\date       10/03/2025
\brief      This reflects the brief
Copyright (C) 2025 TwoJumpingRabbits
*/
/* End Header **************************************************************************/

#pragma once
#include "Systems.h"
#include "Matrix4x4.h"
#include "Window.h"

namespace Ermine::graphics
{
    class Camera : public System
    {
        Mtx44 viewMatrix;
        Mtx44 projectionMatrix;
        EntityID activeCameraEntity;
    public:
        void Update(GLFWwindow* windowContext);

        Mtx44 GetViewMatrix() const { return viewMatrix; }
        Mtx44 GetProjectionMatrix() const { return projectionMatrix; }
        EntityID GetActiveCameraEntity() const { return activeCameraEntity; }

        Vector3D CalculateForwardVector(const Vector3D& rotation) const;
        Vector3D CalculateUpVector(const Vector3D& rotation) const;
    };
}
