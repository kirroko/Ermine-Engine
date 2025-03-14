/* Start Header ************************************************************************/
/*!
\file       Components.h
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong\@digipen.edu
\date       Jan 24, 2025
\brief      Here is where we store all the different components that are needed to be added or removed (i.e Transform, Sprite, etc).

Copyright (C) 2024 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once

#include <utility>

#include "PreCompile.h"
#include "Matrix4x4.h" // Vector3D included

#include "Shader.h"
#include "VertexArray.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Texture.h"

namespace Ermine
{
	/*!***********************************************************************
	\brief
	 Transform component structure.
	*************************************************************************/
	struct Transform
	{
		Mtx44 transform_matrix {1.0f}; // Identity matrix
		Vec3 position;
		Vec3 rotation; // Euler angles in degrees
		Vec3 scale;

		Transform(const Vec3& pos = Vec3(), const Vec3& rot = Vec3(), const Vec3& scl = Vec3(1.f,1.f,1.f)) : position(pos), rotation(rot), scale(scl)
		{
		}
	};

	/*!***********************************************************************
	\brief
	 Rigidbody2D component structure.
	*************************************************************************/
	struct Rigidbody3D
	{
		// Linear Properties
		Vec3 position{};
		Vec3 velocity{};
		Vec3 acceleration{};
		Vec3 force{};
		float mass{1.f};				// Minimum mass of 1
		float inverse_mass{1.f / mass}; // inverse mass = 1/mass
		float linear_drag{0.9f};		// Adjust to control the friction from [0, 1]

		// Rotational Properties
		float angle{};
		float angular_velocity{};
		float angular_acceleration{};
		float torque{};
		float inertia_mass{1.f};					// Minimum inertia mass of 1
		float inv_inertia_mass{1.f / inertia_mass}; // inverse inertia mass = 1/inertia mass
		float angular_drag{0.9f};					// Adjust to control the friction from [0, 1]

		bool use_gravity{false};  // If true, apply gravity
		bool is_kinematic{false}; // If true, don't apply physics
	};

	struct CameraComponent
	{
		float fov;
		float aspectRatio;
		float nearPlane;
		float farPlane;
		bool isPrimary; // Is this the main camera?

		CameraComponent() = default;
		CameraComponent(float fov = 60.0f, float aspect = 16.0f/9.0f, float nearP = 0.1f, float farP = 1000.0f, bool primary = false) :
		fov(fov), aspectRatio(aspect), nearPlane(nearP), farPlane(farP), isPrimary(primary)
		{
		}
	};

	struct Mesh
	{
		std::shared_ptr<graphics::VertexArray> vertex_array;
		std::shared_ptr<graphics::VertexBuffer> vertex_buffer;
		std::shared_ptr<graphics::IndexBuffer> index_buffer;

		Mesh() = default;
		
		Mesh(const std::shared_ptr<graphics::VertexArray>& vao, const std::shared_ptr<graphics::VertexBuffer>& vbo, const std::shared_ptr<graphics::IndexBuffer>& ibo) :
			vertex_array(vao), vertex_buffer(vbo), index_buffer(ibo)
		{}
	};
	
	/*!***********************************************************************
	\brief
	 Material component structure.
	*************************************************************************/
	struct Material
	{
		std::shared_ptr<graphics::Shader> m_shader;
		// graphics::Texture m_texture;

		Material() = default;
		
		Material(const std::shared_ptr<graphics::Shader>& shader) : m_shader(shader)
		{}
	};
}
