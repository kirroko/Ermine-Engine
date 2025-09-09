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

#include "PreCompile.h"
#include "Matrix4x4.h" // Vector3D included

#include "Shader.h"
#include "VertexArray.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "ScriptInstance.h"
#include "Texture.h"
#include "AudioManager.h"

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

		explicit Transform(const Vec3& pos = Vec3(), const Vec3& rot = Vec3(), const Vec3& scl = Vec3(1.f,1.f,1.f)) : position(pos), rotation(rot), scale(scl)
		{
		}
	};

	/*!***********************************************************************
	\brief
	 Rigidbody3D component structure.
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

		explicit Rigidbody3D(const Vec3& pos = Vec3(), const Vec3& vel = Vec3(), const Vec3& acc = Vec3(), const Vec3& frc = Vec3(), float m = 1.f, float inv_m = 1.f, float lin_drag = 0.9f,
			float ang_drag = 0.9f, bool use_grav = false, bool is_kinem = false) :
			position(pos), velocity(vel), acceleration(acc), force(frc), mass(m), inverse_mass(inv_m), linear_drag(lin_drag), angular_drag(ang_drag), use_gravity(use_grav), is_kinematic(is_kinem)
		{
		}
	};

	struct ObjectMetaData
	{
		std::string name{};
		std::string tag{};

		bool selfActive{ };

		ObjectMetaData() : name("GameObject"), tag("Untagged"), selfActive(true)
		{
		}

		ObjectMetaData(std::string name_, std::string tag_, const bool& active) : name(std::move(name_)), tag(std::move(
			tag_)), selfActive(active)
		{
		}
	};

	struct Script
	{
		std::string m_className;
		std::unique_ptr<scripting::ScriptInstance> m_instance;
		bool m_enabled = true;
		bool m_started = false;

		Script() = default;
		explicit Script(std::string className, EntityID id) : m_className(std::move(className))
		{
			auto sc = std::make_unique<scripting::ScriptClass>(scripting::ScriptClass("", m_className));
			m_instance = std::make_unique<scripting::ScriptInstance>(std::move(sc), id);
		}

		Script(const Script& other) : m_className(other.m_className)
		{
			if (other.m_instance)
			{
				// Re-create the script instance with the same class
				auto sc = std::make_unique<scripting::ScriptClass>(scripting::ScriptClass("", m_className));
				m_instance = std::make_unique<scripting::ScriptInstance>(std::move(sc), other.m_instance->entityID);
			}
		}

		Script& operator=(const Script& other)
		{
			if (this != &other)
			{
				m_className = other.m_className;
				if (other.m_instance)
				{
					// Re-create the script instance with the same class
					auto sc = std::make_unique<scripting::ScriptClass>(scripting::ScriptClass("", m_className));
					m_instance = std::make_unique<scripting::ScriptInstance>(std::move(sc), other.m_instance->entityID);
				}
				else
				{
					m_instance.reset();
				}
			}
			return *this;
		}

		Script(Script&& other) noexcept : m_className(std::move(other.m_className)),
		m_instance(std::move(other.m_instance))
		{}

		Script& operator=(Script&& other) noexcept
		{
			if (this != &other)
			{
				m_className = std::move(other.m_className);
				m_instance = std::move(other.m_instance);
			}
			return *this;
		}
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
		std::shared_ptr<graphics::Texture> m_texture;

		Material() = default;
		
		Material(const std::shared_ptr<graphics::Shader>& shader, const std::shared_ptr<graphics::Texture>& texture) : m_shader(shader), m_texture(texture)
		{}
	};

	/*!***********************************************************************
	\brief
	AudioSource structure for individual audio files.
	*************************************************************************/
	struct AudioSource
	{
		std::string audioPath{};
		std::string audioName{};
		float volume{ 0.2f }; // Volume from 0.0f to 1.0f (matches your previous engine)

		AudioSource() = default;
		AudioSource(const std::string& name, const std::string& path, float vol = 0.2f) :
			audioName(name), audioPath(path), volume(vol) {
		}
	};

	/*!***********************************************************************
	\brief
	 Global AudioManager component for managing music, SFX collections, and global audio.
	 Similar to your previous engine but adapted for FMOD.
	*************************************************************************/
	struct GlobalAudioComponent
	{
		std::vector<AudioSource> music; // Music category
		std::vector<AudioSource> sfx;   // SFX category

		// Global volume controls
		float masterVolume{ 1.0f };
		float musicVolume{ 1.0f };
		float sfxVolume{ 1.0f };

		// Currently playing tracks
		int currentMusicIndex{ -1 };
		int currentMusicChannelId{ -1 };

		GlobalAudioComponent() = default;

		// Music management
		void PlayMusic(int index);
		void StopMusic();
		void SetMusicVolume(float volume);

		// SFX management  
		void PlaySFX(int index);
		void PlaySFX(const std::string& name);
		void SetSFXVolume(float volume);

		// Utility functions
		int GetSFXIndex(const std::string& name) const;
		int GetMusicIndex(const std::string& name) const;
		void AddMusicSource(const std::string& name, const std::string& path);
		void AddSFXSource(const std::string& name, const std::string& path);
	};

	/*!***********************************************************************
	\brief
	 Individual AudioComponent for entity-specific audio (footsteps, weapon sounds, etc.)
	 Works alongside the global AudioManager.
	*************************************************************************/
	struct AudioComponent
	{
		// Basic audio properties
		std::string soundName{};
		std::string eventName{}; // For FMOD Studio events

		// Playback control
		int channelId{ -1 }; // Managed by CAudioEngine
		bool isPlaying{ false };
		bool shouldPlay{ false }; // Trigger flag for AudioSystem
		bool shouldStop{ false }; // Trigger flag for AudioSystem

		// Audio settings
		bool is3D{ true };
		bool isLooping{ false };
		bool isStreaming{ false };
		float volume{ 0.0f }; // Volume in dB (-60 to 0) - matches your FMOD system

		// 3D Audio properties
		bool followTransform{ true }; // Should audio follow entity position?
		float minDistance{ 1.0f }; // 3D audio rolloff settings
		float maxDistance{ 100.0f };

		// FMOD Studio event parameters (optional)
		std::map<std::string, float> eventParameters{};

		// Constructors
		AudioComponent() = default;

		explicit AudioComponent(const std::string& sound, bool is3d = true, bool loop = false, float vol = 0.0f) :
			soundName(sound), is3D(is3d), isLooping(loop), volume(vol) {
		}

		explicit AudioComponent(const std::string& event) :
			eventName(event), is3D(false) {
		} // Events typically handle their own 3D settings
	};

	/*!***********************************************************************
	 \brief
	 Particle component structure.
	*************************************************************************/
	struct Particle
	{
		Vec3 velocity;
		float lifetime;
		float age;
		Vec4 colour;
		float size;

		Particle() : velocity(0, 0, 0), lifetime(1.0f), age(0.0f), colour(1, 1, 1, 1), size(1.0f) {}
	};
}
