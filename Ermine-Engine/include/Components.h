/* Start Header ************************************************************************/
/*!
\file       Components.h
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong\@digipen.edu (85%)
\co-author  Jeremy Lim Ting Jie, jeremytingjie.lim, 2301370, jeremytingjie.lim\@digipen.edu (10%)
\co-author  Ridhwan (5%)
\date       Jan 24, 2025
\brief      Updated components with modular material system

Copyright (C) 2024 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once

#include "PreCompile.h"
#include "MathVector.h" // Vector3D included

//#include "Shader.h"
#include "VertexArray.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "ScriptInstance.h"
#include "Texture.h"
#include "Material.h"
#include "AudioManager.h"
#include "Model.h"

namespace Ermine
{
	/*!***********************************************************************
	\brief
	 Transform component structure.
	*************************************************************************/
	struct Transform
	{
		Mtx44 transform_matrix{ 1.0f }; // Identity matrix
		Vec3 position;
		Quaternion rotation; // Euler angles in degrees
		Vec3 scale;

		explicit Transform(const Vec3& pos = Vec3(), const Quaternion& rot = Quaternion(), const Vec3& scl = Vec3(1.f, 1.f, 1.f)) : position(pos), rotation(rot), scale(scl)
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
		float mass{ 1.f };				// Minimum mass of 1
		float inverse_mass{ 1.f / mass }; // inverse mass = 1/mass
		float linear_drag{ 0.9f };		// Adjust to control the friction from [0, 1]

		// Rotational Properties
		float angle{};
		float angular_velocity{};
		float angular_acceleration{};
		float torque{};
		float inertia_mass{ 1.f };					// Minimum inertia mass of 1
		float inv_inertia_mass{ 1.f / inertia_mass }; // inverse inertia mass = 1/inertia mass
		float angular_drag{ 0.9f };					// Adjust to control the friction from [0, 1]

		bool use_gravity{ false };  // If true, apply gravity
		bool is_kinematic{ false }; // If true, don't apply physics

		explicit Rigidbody3D(const Vec3& pos = Vec3(), const Vec3& vel = Vec3(), const Vec3& acc = Vec3(), const Vec3& frc = Vec3(), float m = 1.f, float inv_m = 1.f, float lin_drag = 0.9f,
			float ang_drag = 0.9f, bool use_grav = false, bool is_kinem = false) :
			position(pos), velocity(vel), acceleration(acc), force(frc), mass(m), inverse_mass(inv_m), linear_drag(lin_drag), angular_drag(ang_drag), use_gravity(use_grav), is_kinematic(is_kinem)
		{
		}
	};

	/*!***********************************************************************
	\brief
	 Object Meta Data structure
	*************************************************************************/
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

	/*!***********************************************************************
	\brief
	 Script structure
	*************************************************************************/
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
		{
		}

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

	/*!***********************************************************************
	\brief
	 Camera component structure
	*************************************************************************/
	struct CameraComponent
	{
		float fov;
		float aspectRatio;
		float nearPlane;
		float farPlane;
		bool isPrimary; // Is this the main camera?

		CameraComponent() = default;
		CameraComponent(float fov = 60.0f, float aspect = 16.0f / 9.0f, float nearP = 0.1f, float farP = 1000.0f, bool primary = false) :
			fov(fov), aspectRatio(aspect), nearPlane(nearP), farPlane(farP), isPrimary(primary)
		{
		}
	};

	/*!***********************************************************************
	\brief
	 Mesh structure
	*************************************************************************/
	struct Mesh
	{
		std::shared_ptr<graphics::VertexArray> vertex_array;
		std::shared_ptr<graphics::VertexBuffer> vertex_buffer;
		std::shared_ptr<graphics::IndexBuffer> index_buffer;

		Mesh() = default;

		Mesh(const std::shared_ptr<graphics::VertexArray>& vao, const std::shared_ptr<graphics::VertexBuffer>& vbo, const std::shared_ptr<graphics::IndexBuffer>& ibo) :
			vertex_array(vao), vertex_buffer(vbo), index_buffer(ibo)
		{
		}
	};

	/*!***********************************************************************
	\brief
	 Material component structure
	*************************************************************************/
	struct Material
	{
		//material class
		std::shared_ptr<graphics::Material> m_material;

		Material() = default;

		/**
		 * @brief Constructor taking a modular material.
		 * @param material A shared pointer to a `graphics::Material` object that will be used to initialize the Material.
		 */
		Material(std::shared_ptr<graphics::Material> material) : m_material(std::move(material))
		{
		}

		/**
		 * @brief Legacy constructor for backwards compatibility.
		 * @param shader The shader to associate with the material.
		 * @param texture The texture to associate with the material (optional). If valid, it is set as the albedo map and a fallback texture.
		 */
		Material(const std::shared_ptr<graphics::Shader>& shader, const std::shared_ptr<graphics::Texture>& texture)
		{
			m_material = std::make_shared<graphics::Material>(shader);
			if (texture && texture->IsValid())
			{
				m_material->SetTexture("material.albedoMap", texture);
				m_material->SetTexture("texture0", texture); // Fallback for old shaders
			}

			// Set default PBR values
			m_material->LoadTemplate(graphics::MaterialTemplates::PBR_RED());
		}


		/**
		 * @brief Copy constructor for the Material class.
		 * @param other The other Material object to copy from.
		 */
		Material(const Material& other) : m_material(other.m_material)
		{
			// Shared ownership - multiple entities can share the same material
		}

		/**
		 * @brief Copy assignment operator for the Material class.
		 * @param other The other Material object to copy from.
		 * @return A reference to this Material object after the copy assignment.
		 */
		Material& operator=(const Material& other)
		{
			if (this != &other)
			{
				m_material = other.m_material; // Shared ownership
			}
			return *this;
		}

		/**
		 * @brief Move constructor for the Material class.
		 * @param other The Material object to move from.
		 */
		Material(Material&& other) noexcept : m_material(std::move(other.m_material))
		{
		}

		/**
		 * @brief Move assignment operator for the Material class.
		 * @param other The Material object to move from.
		 * @return A reference to this Material object after the move assignment.
		 */
		Material& operator=(Material&& other) noexcept
		{
			if (this != &other)
			{
				m_material = std::move(other.m_material);
			}
			return *this;
		}

		/**
		 * @brief Retrieves the raw `graphics::Material` pointer.
		 * @return A pointer to the internal `graphics::Material` object.
		 */
		graphics::Material* GetMaterial() const {
			return m_material.get();
		}

		/**
		 * @brief Get the shared material pointer for sharing between entities.
		 * @return A shared pointer to the internal `graphics::Material` object.
		 */
		std::shared_ptr<graphics::Material> GetSharedMaterial() const {
			return m_material;
		}

		/**
		* @brief Sets the albedo color for the material.
		* @details Albedo represents the diffuse color of the material.
		* @param albedo A Vec3 representing the RGB color value for the albedo.
		*/
		void SetAlbedo(const Vec3& albedo)
		{
			if (m_material) m_material->SetVec3("material.albedo", albedo);
		}

		/**
		* @brief Sets the roughness value for the material.
		* @details Roughness defines the material's surface smoothness. A value of 0.0 is smooth, and 1.0 is rough.
		* @param roughness A float representing the roughness of the material.
		*/
		void SetRoughness(float roughness)
		{
			if (m_material) m_material->SetFloat("material.roughness", roughness);
		}

		/**
		* @brief Sets the metallic value for the material.
		* @details Metallic defines whether the material is metallic or dielectric. A value of 1.0 means fully metallic.
		* @param metallic A float representing the metallic property of the material.
		*/
		void SetMetallic(float metallic)
		{
			if (m_material) m_material->SetFloat("material.metallic", metallic);
		}

		/**
		* @brief Sets the emissive color for the material.
		* @details Emissive represents the material's self-illumination. It can be used to simulate glowing materials.
		* @param emissive A Vec3 representing the RGB color of the emissive property.
		* @param intensity A float value controlling the intensity of the emissive property (default: 1.0).
		*/
		void SetEmissive(const Vec3& emissive, float intensity = 1.0f)
		{
			if (m_material)
			{
				m_material->SetVec3("material.emissive", emissive);
				m_material->SetFloat("material.emissiveIntensity", intensity);
			}
		}

		/**
		* @brief Sets the normal map for the material.
		* @details The normal map is used to simulate small surface details like bumps and dents.
		* @param normalMap Shared pointer to a valid Texture representing the normal map.
		*/
		void SetNormalMap(std::shared_ptr<graphics::Texture> normalMap)
		{
			if (m_material && normalMap && normalMap->IsValid())
			{
				m_material->SetTexture("material.normalMap", normalMap);
				m_material->SetBool("material.hasNormalMap", true);
			}
		}
	};

	/*!***********************************************************************
	\brief
	 Light type structure
	*************************************************************************/
	enum class LightType : int
	{
		POINT = 0,
		DIRECTIONAL = 1,
		SPOT = 2
	};

	/*!***********************************************************************
	\brief
	 Light structure
	*************************************************************************/
	struct Light {
		Vec3 color;
		float intensity;
		LightType type;
		bool castsShadows{ false };
		vector<glm::mat4> lightSpaceMatrices{ glm::mat4{0}, glm::mat4{0}, glm::mat4{0}, glm::mat4{0} }; // For shadow mapping
		int startOffset{ 0 }; // For UBO indexing
		float innercos{ -1.0f }; // For spotlights
		float outercos{ -1.0f }; // For spotlights
		float radius{ 1.0f }; // For point lights
		glm::vec4 splitDepths{ 0.1f, 10.0f, 50.0f , 100.f}; // For Cascaded Shadow Maps (CSM)

		Light() : color(1.0f, 1.0f, 1.0f),
			intensity(1.0f),
			type(LightType::POINT)
		{
		}

		Light(const Vec3& col, float intens, LightType t) :
			color(col), intensity(intens), type(t)
		{
		}

		Light(const Vec3& col, float intens, LightType t, bool shadows) :
			color(col), intensity(intens), type(t), castsShadows(shadows)
		{
		}

		Light(const Vec3& col, float intens, LightType t, bool shadows, float inner, float outer, float rad = 1.0f) :
			color(col), intensity(intens), type(t), castsShadows(shadows), innercos(inner), outercos(outer), radius(rad)
		{
		
		}
	};
	
	/*!***********************************************************************
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
		float volume{ 0.5f }; // Volume from 0.0f to 1.0f (NOT dB!) - will be converted to dB when needed

		// 3D Audio properties
		bool followTransform{ true }; // Should audio follow entity position?
		float minDistance{ 1.0f }; // 3D audio rolloff settings
		float maxDistance{ 100.0f };

		// FMOD Studio event parameters (optional)
		std::map<std::string, float> eventParameters{};

		// Constructors
		AudioComponent() = default;
		explicit AudioComponent(const std::string& sound, bool is3d = true, bool loop = false, float vol = 0.5f) :
			soundName(sound), is3D(is3d), isLooping(loop), volume(vol) {
		}
		explicit AudioComponent(const std::string& event) :
			eventName(event), is3D(false), volume(0.5f) {
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

	/*!***********************************************************************
	 \brief
	 Reflection Probe component for local environment mapping
	*************************************************************************/
	struct ReflectionProbe
	{
		// Probe properties
		Vec3 position{0.0f, 0.0f, 0.0f};
		Vec3 size{10.0f, 10.0f, 10.0f};       // Bounding box size
		float intensity{1.0f};
		float blendDistance{1.0f};             // Distance over which to blend with other probes
		bool boxProjection{true};              // Use box projection for more accurate reflections
		
		// Runtime cubemap data
		std::shared_ptr<graphics::Cubemap> reflectionCubemap{nullptr};
		std::shared_ptr<graphics::Cubemap> irradianceCubemap{nullptr};
		
		// Probe influence settings
		bool isActive{true};
		int priority{0};                       // Higher priority probes override lower ones
		
		// Box projection parameters (for accurate local reflections)
		Vec3 boxMin{-5.0f, -5.0f, -5.0f};    // Local space bounding box
		Vec3 boxMax{5.0f, 5.0f, 5.0f};
		
		ReflectionProbe() = default;
		
		/**
		 * @brief Constructor with basic setup
		 * @param pos Position of the probe in world space
		 * @param probeSize Size of the influence area
		 * @param probeIntensity Intensity multiplier for reflections
		 */
		ReflectionProbe(const Vec3& pos, const Vec3& probeSize, float probeIntensity = 1.0f)
			: position(pos), size(probeSize), intensity(probeIntensity)
		{
			// Set bounding box to match size
			boxMin = -probeSize * 0.5f;
			boxMax = probeSize * 0.5f;
		}
		
		/**
		 * @brief Check if a world position is within this probe's influence
		 * @param worldPos Position to test
		 * @return True if position is influenced by this probe
		 */
		bool IsWithinInfluence(const Vec3& worldPos) const
		{
			Vec3 localPos = worldPos - position;
			return (localPos.x >= boxMin.x && localPos.x <= boxMax.x &&
					localPos.y >= boxMin.y && localPos.y <= boxMax.y &&
					localPos.z >= boxMin.z && localPos.z <= boxMax.z);
		}
		
		/**
		 * @brief Calculate blend weight for a world position
		 * @param worldPos Position to calculate weight for
		 * @return Weight between 0 and 1
		 */
		float CalculateWeight(const Vec3& worldPos) const
		{
			if (!IsWithinInfluence(worldPos)) return 0.0f;
			
			Vec3 localPos = worldPos - position;
			Vec3 distToEdge = Vec3(
				std::min(localPos.x - boxMin.x, boxMax.x - localPos.x),
				std::min(localPos.y - boxMin.y, boxMax.y - localPos.y),
				std::min(localPos.z - boxMin.z, boxMax.z - localPos.z)
			);
			
			float minDist = std::min({distToEdge.x, distToEdge.y, distToEdge.z});
			
			if (minDist >= blendDistance) return 1.0f;
			return minDist / blendDistance;
		}
	};
	
	/*!***********************************************************************
	\brief
	 Model component structure.
	*************************************************************************/
	struct ModelComponent
	{
		std::shared_ptr<graphics::Model> m_model;

		ModelComponent() = default;
		explicit ModelComponent(const std::shared_ptr<graphics::Model>& model) : m_model(model) {}
	};
}
