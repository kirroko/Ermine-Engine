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

#include <glm/ext/matrix_transform.hpp>

#include "ECS.h"
#include "FrameController.h"
#include "Input.h"
#include "MathUtils.h"
#include "Renderer.h"
#include "GLFW/glfw3.h"

using namespace Ermine::editor;

/**
 * @brief Construct a new Editor Camera object
 * @param fov The field of view
 * @param aspectRatio The aspect ratio
 * @param near The near plane
 * @param far The far plane
 */
EditorCamera::EditorCamera(float fov, float aspectRatio, float nearClip, float farClip)
	: m_FOV(fov), m_AspectRatio(aspectRatio), m_NearClip(nearClip), m_FarClip(farClip)
{
	UpdateViewMatrix();
	UpdateProjectionMatrix();
}

/**
* @brief Update the camera based on input and time between frames
* @param deltaTime The time between frames
*/
void EditorCamera::Update()
{
	UpdateViewMatrix();
	UpdateProjectionMatrix();
}

/**
* @brief Set the perspective of the camera
* @param fov The field of view
* @param aspectRatio The aspect ratio
* @param nearClip The near plane
* @param farClip The far plane
*/
void EditorCamera::SetPerspective(float fov, float aspectRatio, float nearClip, float farClip)
{
	m_FOV = fov;
	m_AspectRatio = aspectRatio;
	m_NearClip = nearClip;
	m_FarClip = farClip;
	UpdateProjectionMatrix();
}

/**
* @brief Set the viewport size
* @param width The width of the viewport
* @param height The height of the viewport
*/
void EditorCamera::SetViewportSize(float width, float height)
{
	m_AspectRatio = width / height;

#ifdef _DEBUG
	// Update the frame buffer size for drawing on screen
	ECS::GetInstance().GetSystem<graphics::Renderer>()->Create(static_cast<int>(width), static_cast<int>(height));
#endif

	UpdateProjectionMatrix();
}

/**
* @brief Update the view matrix based on the camera's position and orientation
*/
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
	// TODO: Change this to in built math function
	glm::mat4 tempLookAtMatrix = glm::lookAt(glm::vec3(m_Position.x, m_Position.y, m_Position.z), glm::vec3(m_Position.x + m_Front.x, m_Position.y + m_Front.y, m_Position.z + m_Front.z), glm::vec3(m_Up.x, m_Up.y, m_Up.z));
	m_ViewMatrix = Mtx44(&tempLookAtMatrix[0][0]);
	// Mtx44LookAt(m_ViewMatrix, m_Position, m_Position + m_Front, m_Up);
}

/**
* @brief Update the projection matrix based on the camera's perspective parameters
*/
void EditorCamera::UpdateProjectionMatrix()
{
	Mtx44Perspective(m_ProjectionMatrix, m_FOV, m_AspectRatio, m_NearClip, m_FarClip);
}

/**
* @brief Process keyboard input to move the camera
* @param deltaTime The time between frames
*/
void EditorCamera::ProcessKeyboardInput(float deltaTime)
{
	if (!Input::IsMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT))
		return;

	float velocity = m_MovementSpeed * deltaTime;

	if (Input::IsKeyDown(GLFW_KEY_W))
		m_Position = m_Position + m_Front * velocity;
	if (Input::IsKeyDown(GLFW_KEY_S))
		m_Position = m_Position - m_Front * velocity;
	if (Input::IsKeyDown(GLFW_KEY_A))
		m_Position = m_Position - m_Right * velocity;
	if (Input::IsKeyDown(GLFW_KEY_D))
		m_Position = m_Position + m_Right * velocity;
	if (Input::IsKeyDown(GLFW_KEY_Q))
		m_Position = m_Position - m_WorldUp * velocity;
	if (Input::IsKeyDown(GLFW_KEY_E))
		m_Position = m_Position + m_WorldUp * velocity;
}

/**
* @brief Process mouse movement to rotate the camera
*/
void EditorCamera::ProcessMouseMovement()
{
	// Only rotate camera if right mouse button is pressed
	if (Input::IsMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT))
	{
		double xpos = Input::GetMouseX(), ypos = Input::GetMouseY();

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

/**
* @brief Process scroll wheel to zoom the camera
* @param yOffset The offset of the scroll wheel
*/
void EditorCamera::ProcessScrollWheel(float yOffset)
{
	m_FOV -= yOffset * FrameController::GetFixedDeltaTime();
	m_FOV = std::max(m_FOV, 1.0f);
	m_FOV = std::min(m_FOV, 45.0f);
	Input::ResetMouseScrollOffset();
}