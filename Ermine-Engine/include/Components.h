/* Start Header ************************************************************************/
/*!
\file       Components.h
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong\@digipen.edu (65%)
\co-author  Jeremy Lim Ting Jie, jeremytingjie.lim, 2301370, jeremytingjie.lim\@digipen.edu (10%)
\co-author  Ridhwan (5%)
\co-author  Curtis (20%)
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
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include "Model.h"
#include "Animator.h"
#include "AssetManager.h"
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/Body.h>

namespace Ermine
{
	inline Quaternion QuaternionFromEulerDegrees(const Vec3& eulerDeg) {
		// Convert degrees to radians
		float pitch = glm::radians(eulerDeg.x); // or your own math::ToRadians
		float yaw = glm::radians(eulerDeg.y);
		float roll = glm::radians(eulerDeg.z);

		float cy = cosf(yaw * 0.5f);

		float sy = sinf(yaw * 0.5f);
		float cp = cosf(pitch * 0.5f);
		float sp = sinf(pitch * 0.5f);
		float cr = cosf(roll * 0.5f);
		float sr = sinf(roll * 0.5f);

		Quaternion q{};
		q.w = cr * cp * cy + sr * sp * sy;
		q.x = sr * cp * cy - cr * sp * sy;
		q.y = cr * sp * cy + sr * cp * sy;
		q.z = cr * cp * sy - sr * sp * cy;
		return q;
	}

	inline Vec3 JsonToVec3(const rapidjson::Value& a) {
		return a.IsArray() && a.Size() == 3
			? Vec3(a[0].GetFloat(), a[1].GetFloat(), a[2].GetFloat())
			: Vec3();
	}

	inline rapidjson::Value Vec4ToJson(const Vec4& v, rapidjson::Document::AllocatorType& alloc) {
		rapidjson::Value a(rapidjson::kArrayType);
		a.PushBack(v.x, alloc).PushBack(v.y, alloc).PushBack(v.z, alloc).PushBack(v.w, alloc);
		return a;
	}

	inline Vec4 JsonToVec4(const rapidjson::Value& arr) {
		if (arr.IsArray() && arr.Size() == 4) {
			return Vec4(arr[0].GetFloat(), arr[1].GetFloat(), arr[2].GetFloat(), arr[3].GetFloat());
		}
		return Vec4(0.f, 0.f, 0.f, 0.f); // default fallback
	}

	inline rapidjson::Value Vec3ToJson(const Vec3& v, rapidjson::Document::AllocatorType& alloc) {
		rapidjson::Value a(rapidjson::kArrayType);
		a.PushBack(v.x, alloc).PushBack(v.y, alloc).PushBack(v.z, alloc);
		return a;
	}

	inline rapidjson::Value QuatToJson(const Quaternion& q, rapidjson::Document::AllocatorType& alloc) {
		rapidjson::Value a(rapidjson::kArrayType);
		a.PushBack(q.w, alloc).PushBack(q.x, alloc).PushBack(q.y, alloc).PushBack(q.z, alloc);
		return a;
	}


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

		template<typename Alloc>
		void Serialize(rapidjson::Value& out, Alloc& alloc) const {
			out.SetObject();

			out.AddMember("position", Vec3ToJson(position, alloc), alloc);
			out.AddMember("rotation", QuatToJson(rotation, alloc), alloc);
			out.AddMember("scale", Vec3ToJson(scale, alloc), alloc);
		}

		void Deserialize(const rapidjson::Value& in) {
			//auto json_to_vec3 = [&](const rapidjson::Value& arr) {
			//	return Vector3D(arr[0].GetFloat(), arr[1].GetFloat(), arr[2].GetFloat());
			//	};

			if (in.HasMember("position") && in["position"].IsArray() && in["position"].Size() == 3)
				position = JsonToVec3(in["position"]);

			if (in.HasMember("scale") && in["scale"].IsArray() && in["scale"].Size() == 3)
				scale = JsonToVec3(in["scale"]);

			if (in.HasMember("rotation") && in["rotation"].IsArray()) {
				const auto& r = in["rotation"];
				if (r.Size() == 4) {
					// Expecting [w, x, y, z]
					rotation.w = r[0].GetFloat();
					rotation.x = r[1].GetFloat();
					rotation.y = r[2].GetFloat();
					rotation.z = r[3].GetFloat();
				}
				else if (r.Size() == 3) {
					Vec3 eulerDeg = JsonToVec3(r);
					rotation = QuaternionFromEulerDegrees(eulerDeg);
				}
			}
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

		// 
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

		template<typename Alloc>
		void Serialize(rapidjson::Value& out, Alloc& alloc) const {
			out.SetObject();
			out.AddMember("name", rapidjson::Value(name.c_str(), alloc), alloc);
			out.AddMember("tag", rapidjson::Value(tag.c_str(), alloc), alloc);
			out.AddMember("active", selfActive, alloc);
		}

		void Deserialize(const rapidjson::Value& in) {
			if (in.HasMember("name") && in["name"].IsString())   name = in["name"].GetString();
			if (in.HasMember("tag") && in["tag"].IsString())    tag = in["tag"].GetString();
			if (in.HasMember("active") && in["active"].IsBool()) selfActive = in["active"].GetBool();
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

		template <typename Alloc>
		void Serialize(rapidjson::Value& out, Alloc& alloc) const {
			out.SetObject();
			out.AddMember("class", rapidjson::Value(m_className.c_str(), alloc), alloc);
			out.AddMember("enabled", m_enabled, alloc);
		}

		void Deserialize(const rapidjson::Value& in) {
			if (in.HasMember("class") && in["class"].IsString()) m_className = in["class"].GetString();
			if (in.HasMember("enabled") && in["enabled"].IsBool()) m_enabled = in["enabled"].GetBool();
			// Note: re-create ScriptInstance when attaching to entity (needs EntityID)
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

		template <typename Alloc>
		void Serialize(rapidjson::Value& out, Alloc& alloc) const {
			out.SetObject();
			out.AddMember("fov", fov, alloc);
			out.AddMember("aspect", aspectRatio, alloc);
			out.AddMember("near", nearPlane, alloc);
			out.AddMember("far", farPlane, alloc);
			out.AddMember("primary", isPrimary, alloc);
		}

		void Deserialize(const rapidjson::Value& in) {
			if (in.HasMember("fov"))       fov = in["fov"].GetFloat();
			if (in.HasMember("aspect"))    aspectRatio = in["aspect"].GetFloat();
			if (in.HasMember("near"))      nearPlane = in["near"].GetFloat();
			if (in.HasMember("far"))       farPlane = in["far"].GetFloat();
			if (in.HasMember("primary"))   isPrimary = in["primary"].GetBool();
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

		enum class Kind { None, Primitive, Asset };
		Kind kind = Kind::None;

		// Primitive description (enough for your CreateCube)
		struct PrimitiveDesc { std::string type; Vec3 size{ 1,1,1 }; } primitive;

		// Asset description
		struct AssetDesc { std::string meshName; } asset;

		Mesh() = default;

		Mesh(const std::shared_ptr<graphics::VertexArray>& vao, const std::shared_ptr<graphics::VertexBuffer>& vbo, const std::shared_ptr<graphics::IndexBuffer>& ibo) :
			vertex_array(vao), vertex_buffer(vbo), index_buffer(ibo)
		{
		}

		void RebuildPrimitive(); // implemented in .cpp
		//void Deserialize(const rapidjson::Value& in) {
		//	// ... set kind/primitive.type/primitive.size as you already do ...
		//	if (kind == Kind::Primitive) RebuildPrimitive();
		//	// Asset case: your loader, etc.
		//}

		template <typename Alloc>
		void Serialize(rapidjson::Value& out, Alloc& alloc) const {
			out.SetObject();
			switch (kind) {
			case Kind::Primitive: {
				out.AddMember("kind", "Primitive", alloc);
				rapidjson::Value p(rapidjson::kObjectType);
				p.AddMember("type", rapidjson::Value(primitive.type.c_str(), alloc), alloc);
				p.AddMember("size", Vec3ToJson(primitive.size, alloc), alloc);
				out.AddMember("primitive", p, alloc);
				break;
			}
			case Kind::Asset: {
				out.AddMember("kind", "Asset", alloc);
				rapidjson::Value a(rapidjson::kObjectType);
				a.AddMember("meshName", rapidjson::Value(asset.meshName.c_str(), alloc), alloc);
				out.AddMember("asset", a, alloc);
				break;
			}
			default:
				out.AddMember("kind", "None", alloc);
				break;
			}
		}

		void Deserialize(const rapidjson::Value& in) {
			if (!in.HasMember("kind")) { kind = Kind::None; return; }
			const auto& k = in["kind"];
			if (k.IsString()) {
				if (strcmp(k.GetString(), "Primitive") == 0) {
					kind = Kind::Primitive;
					const auto& p = in["primitive"];
					if (p.HasMember("type") && p["type"].IsString()) primitive.type = p["type"].GetString();
					if (p.HasMember("size")) primitive.size = JsonToVec3(p["size"]);

					RebuildPrimitive();
					// extend for other primitives
				}

				else if (strcmp(k.GetString(), "Asset") == 0) {
					kind = Kind::Asset;
					const auto& a = in["asset"];
					if (a.HasMember("meshName") && a["meshName"].IsString()) asset.meshName = a["meshName"].GetString();
					// Rebuild from asset if you have a mesh loader separate from Model
				}
				else {
					kind = Kind::None;
				}
			}
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

		// ---- Minimal cached authoring values for serialization ----
		std::string materialTemplate;       // optional
		bool hasAlbedo = false;   Vec3  cacheAlbedo{ 1,1,1 };
		bool hasRough = false;   float cacheRoughness = 0.5f;
		bool hasMetal = false;   float cacheMetallic = 0.0f;
		bool hasEmiss = false;   Vec3  cacheEmissive{ 0,0,0 };
		float cacheEmissiveIntensity = 1.0f;

		//// Cached texture paths (only what we set by path)
		//bool hasAlbedoMapPath = false;   std::string albedoMapPath;
		//bool hasNormalMapPath = false;   std::string normalMapPath;

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
			hasAlbedo = true; cacheAlbedo = albedo;
			if (m_material) m_material->SetVec3("material.albedo", albedo);
		}

		/**
		* @brief Sets the roughness value for the material.
		* @details Roughness defines the material's surface smoothness. A value of 0.0 is smooth, and 1.0 is rough.
		* @param roughness A float representing the roughness of the material.
		*/
		void SetRoughness(float roughness)
		{
			hasRough = true; cacheRoughness = roughness;
			if (m_material) m_material->SetFloat("material.roughness", roughness);
		}

		/**
		* @brief Sets the metallic value for the material.
		* @details Metallic defines whether the material is metallic or dielectric. A value of 1.0 means fully metallic.
		* @param metallic A float representing the metallic property of the material.
		*/
		void SetMetallic(float metallic)
		{
			hasMetal = true; cacheMetallic = metallic;
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
			hasEmiss = true; cacheEmissive = emissive; cacheEmissiveIntensity = intensity;
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

		template <typename Alloc>
		void Serialize(rapidjson::Value& out, Alloc& alloc) const {
			out.SetObject();

			// No material => nothing to serialize
			if (!m_material) {
				out.AddMember("hasMaterial", false, alloc);
				return;
			}
			out.AddMember("hasMaterial", true, alloc);

			// Serialize selected scalar/vector params so UBO can be restored
			rapidjson::Value params(rapidjson::kObjectType);

			auto writeFloat = [&](const char* name) {
				if (auto p = m_material->GetParameter(name)) {
					if (!p->floatValues.empty()) {
						rapidjson::Value v;
						v.SetFloat(p->floatValues[0]);
						params.AddMember(rapidjson::StringRef(name), v, alloc);
					}
				}
				};

			auto writeInt = [&](const char* name) {
				if (auto p = m_material->GetParameter(name)) {
					rapidjson::Value v;
					v.SetInt(p->intValue);
					params.AddMember(rapidjson::StringRef(name), v, alloc);
				}
				};

			auto writeBool = [&](const char* name) {
				if (auto p = m_material->GetParameter(name)) {
					rapidjson::Value v;
					v.SetBool(p->boolValue);
					params.AddMember(rapidjson::StringRef(name), v, alloc);
				}
				};

			auto writeVec3 = [&](const char* name) {
				if (auto p = m_material->GetParameter(name); p && p->floatValues.size() >= 3) {
					rapidjson::Value a(rapidjson::kArrayType);
					a.PushBack(p->floatValues[0], alloc)
						.PushBack(p->floatValues[1], alloc)
						.PushBack(p->floatValues[2], alloc);
					params.AddMember(rapidjson::StringRef(name), a, alloc);
				}
				};

			// Core PBR parameters (names follow graphics::Material templates)
			writeVec3("materialAlbedo");
			writeFloat("materialMetallic");
			writeFloat("materialRoughness");
			writeFloat("materialAo");
			writeVec3("materialEmissive");
			writeFloat("materialEmissiveIntensity");
			writeFloat("materialNormalStrength");
			writeInt("materialShadingModel");
			writeFloat("materialReflectance");
			writeFloat("materialEnvironmentIntensity");

			// Map presence flags
			writeBool("materialHasAlbedoMap");
			writeBool("materialHasNormalMap");
			writeBool("materialHasRoughnessMap");
			writeBool("materialHasMetallicMap");
			writeBool("materialHasAoMap");
			writeBool("materialHasEmissiveMap");
			writeBool("materialHasEnvironmentMap");
			writeBool("materialHasIrradianceMap");

			out.AddMember("params", params, alloc);

			// Serialize textures: store slot name and source file path
			rapidjson::Value textures(rapidjson::kArrayType);

			// Known slots across the codebase (support both dot and non-dot styles + fallback)
			const char* slots[] = {
				"materialAlbedoMap", "material.albedoMap",
				"material.normalMap",
				"materialRoughnessMap", "material.metallicMap", "materialAoMap", "materialEmissiveMap",
				"texture0" // fallback for legacy
			};

			// Helper: find file path for a given texture via AssetManager cache
			auto findPathForTexture = [](const std::shared_ptr<graphics::Texture>& tex) -> std::string {
				if (!tex) return {};
				for (const auto& kv : AssetManager::GetInstance().GetLoadedTextures()) {
					if (kv.second.get() == tex.get())
						return kv.first;
				}
				return {};
				};

			for (const char* slot : slots) {
				if (auto tex = m_material->GetTexture(slot)) {
					if (tex && tex->IsValid()) {
						std::string path = findPathForTexture(tex);
						if (!path.empty()) {
							rapidjson::Value texObj(rapidjson::kObjectType);
							texObj.AddMember("slot", rapidjson::Value(slot, alloc), alloc);
							texObj.AddMember("path", rapidjson::Value(path.c_str(), alloc), alloc);
							textures.PushBack(texObj, alloc);
						}
					}
				}
			}

			out.AddMember("textures", textures, alloc);
		}

		void Deserialize(const rapidjson::Value& in) {
			// Ensure material exists
			if (!m_material) {
				m_material = std::make_shared<graphics::Material>();
			}
			if (!in.IsObject()) return;
			if (in.HasMember("hasMaterial") && in["hasMaterial"].IsBool() && !in["hasMaterial"].GetBool())
				return;

			// Restore params
			if (in.HasMember("params") && in["params"].IsObject()) {
				const auto& p = in["params"];

				auto readVec3 = [&](const char* name, const char* compatName = nullptr) {
					if (p.HasMember(name) && p[name].IsArray() && p[name].Size() == 3) {
						Vec3 v{ p[name][0].GetFloat(), p[name][1].GetFloat(), p[name][2].GetFloat() };
						m_material->SetVec3(name, v);
						// Also write compatibility alias if provided
						if (compatName) m_material->SetVec3(compatName, v);
					}
					};
				auto readFloat = [&](const char* name, const char* compatName = nullptr) {
					if (p.HasMember(name) && p[name].IsNumber()) {
						m_material->SetFloat(name, p[name].GetFloat());
						if (compatName) m_material->SetFloat(compatName, p[name].GetFloat());
					}
					};
				auto readInt = [&](const char* name) {
					if (p.HasMember(name) && p[name].IsInt())
						m_material->SetInt(name, p[name].GetInt());
					};
				auto readBool = [&](const char* name, const char* compatName = nullptr) {
					if (p.HasMember(name) && p[name].IsBool()) {
						m_material->SetBool(name, p[name].GetBool());
						if (compatName) m_material->SetBool(compatName, p[name].GetBool());
					}
					};
				// Core PBR parameters
				readVec3("materialAlbedo", "material.albedo");
				readFloat("materialMetallic", "material.metallic");
				readFloat("materialRoughness", "material.roughness");
				readFloat("materialAo", "material.ao");
				readVec3("materialEmissive", "material.emissive");
				readFloat("materialEmissiveIntensity", "material.emissiveIntensity");
				readFloat("materialNormalStrength", "material.normalStrength");
				readInt("materialShadingModel");
				readFloat("materialReflectance");
				readFloat("materialEnvironmentIntensity");

				// Map presence flags
				readBool("materialHasAlbedoMap");
				readBool("materialHasNormalMap", "material.hasNormalMap");
				readBool("materialHasRoughnessMap");
				readBool("materialHasMetallicMap");
				readBool("materialHasAoMap");
				readBool("materialHasEmissiveMap");
				readBool("materialHasEnvironmentMap");
				readBool("materialHasIrradianceMap");
			}
			// Restore textures
			if (in.HasMember("textures") && in["textures"].IsArray()) {
				const auto& arr = in["textures"];
				for (auto& t : arr.GetArray()) {
					if (!t.IsObject()) continue;
					if (!t.HasMember("slot") || !t.HasMember("path")) continue;
					if (!t["slot"].IsString() || !t["path"].IsString()) continue;

					std::string slot = t["slot"].GetString();
					std::string path = t["path"].GetString();

					auto tex = AssetManager::GetInstance().LoadTexture(path);
					if (tex && tex->IsValid()) {
						m_material->SetTexture(slot, tex);

						// For compatibility, mirror well-known slots to alternate names
						if (slot == "materialAlbedoMap")
							m_material->SetTexture("material.albedoMap", tex);
						else if (slot == "material.albedoMap")
							m_material->SetTexture("materialAlbedoMap", tex);


						// Update presence flags for known types
						auto setPresence = [&](const char* nonDot, const char* dot) {
							m_material->SetBool(nonDot, true);
							m_material->SetBool(dot, true);
							};
						if (slot.find("normal") != std::string::npos) {
							setPresence("materialHasNormalMap", "material.hasNormalMap");
						}
						if (slot.find("Albedo") != std::string::npos || slot.find("albedo") != std::string::npos) {
							m_material->SetBool("materialHasAlbedoMap", true);
						}
						if (slot.find("Roughness") != std::string::npos || slot.find("roughness") != std::string::npos) {
							m_material->SetBool("materialHasRoughnessMap", true);
						}
						if (slot.find("Metallic") != std::string::npos || slot.find("metallic") != std::string::npos) {
							m_material->SetBool("materialHasMetallicMap", true);
						}
						if (slot.find("Ao") != std::string::npos || slot.find("ao") != std::string::npos || slot.find("AmbientOcclusion") != std::string::npos) {
							m_material->SetBool("materialHasAoMap", true);
						}
						if (slot.find("Emissive") != std::string::npos || slot.find("emissive") != std::string::npos) {
							m_material->SetBool("materialHasEmissiveMap", true);
						}
					}
				}
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
	 Light GPU structure
	*************************************************************************/
	struct LightGPU
	{
		glm::vec4 position_type;    // xyz = position (view space), w = light type
		glm::vec4 color_intensity;  // xyz = color, w = intensity
		glm::vec4 direction_range;  // xyz = direction (view space), w = range
		glm::vec4 spot_angles_castshadows_resolution;      // x = inner cos, y = outer cos z = casts shadows (1.0 or 0.0), w = shadow map resolution
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
		unsigned int resolution{ 1024 }; // Shadow map resolution

		Light() : color(1.0f, 1.0f, 1.0f),
			intensity(1.0f),
			type(LightType::POINT)
		{
		}

		Light(const Vec3& col, float intens, LightType t) :
			color(col), intensity(intens), type(t)
		{
		}

		Light(const Vec3& col, float intens, LightType t, bool shadows, unsigned int res) :
			color(col), intensity(intens), type(t), castsShadows(shadows), resolution(res)
		{
		}

		template<typename Alloc>
		void Serialize(rapidjson::Value& out, Alloc& alloc) const {
			out.SetObject();

			auto vec3_to_json = [&](const Vec3& v) {
				rapidjson::Value a(rapidjson::kArrayType);
				a.PushBack(v.x, alloc).PushBack(v.y, alloc).PushBack(v.z, alloc);
				return a;
				};

			out.AddMember("color", vec3_to_json(color), alloc);
			out.AddMember("intensity", intensity, alloc);
			out.AddMember("type", static_cast<int>(type), alloc);          // 0=POINT,1=DIR,2=SPOT
			out.AddMember("castsShadows", castsShadows, alloc);
			out.AddMember("resolution", resolution, alloc);
		}

		void Deserialize(const rapidjson::Value& in) {
			auto json_to_vec3 = [&](const rapidjson::Value& arr) {
				return Vec3(arr[0].GetFloat(), arr[1].GetFloat(), arr[2].GetFloat());
				};

			if (in.HasMember("color") && in["color"].IsArray() && in["color"].Size() == 3)
				color = json_to_vec3(in["color"]);
			if (in.HasMember("intensity") && in["intensity"].IsNumber())
				intensity = in["intensity"].GetFloat();
			if (in.HasMember("type") && in["type"].IsInt()) {
				int t = in["type"].GetInt();
				if (t == 1) type = LightType::DIRECTIONAL;
				else if (t == 2) type = LightType::SPOT;
				else             type = LightType::POINT;
			}
			if (in.HasMember("castsShadows") && in["castsShadows"].IsBool())
				castsShadows = in["castsShadows"].GetBool();
			if (in.HasMember("resolution") && in["resolution"].IsUint())
				resolution = in["resolution"].GetUint();
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

		// GlobalAudioComponent
		template <typename Alloc>
		void Serialize(rapidjson::Value& out, Alloc& alloc) const {
			out.SetObject();
			out.AddMember("masterVolume", masterVolume, alloc);
			out.AddMember("musicVolume", musicVolume, alloc);
			out.AddMember("sfxVolume", sfxVolume, alloc);

			auto writeList = [&](const std::vector<AudioSource>& list, const char* key) {
				rapidjson::Value arr(rapidjson::kArrayType);
				for (auto& s : list) {
					rapidjson::Value o(rapidjson::kObjectType);
					o.AddMember("name", rapidjson::Value(s.audioName.c_str(), alloc), alloc);
					o.AddMember("path", rapidjson::Value(s.audioPath.c_str(), alloc), alloc);
					o.AddMember("volume", s.volume, alloc);
					arr.PushBack(o, alloc);
				}
				out.AddMember(rapidjson::StringRef(key), arr, alloc);
				};
			writeList(music, "music");
			writeList(sfx, "sfx");
		}
		void Deserialize(const rapidjson::Value& in) {
			if (in.HasMember("masterVolume")) masterVolume = in["masterVolume"].GetFloat();
			if (in.HasMember("musicVolume")) musicVolume = in["musicVolume"].GetFloat();
			if (in.HasMember("sfxVolume"))   sfxVolume = in["sfxVolume"].GetFloat();

			auto readList = [&](const char* key, std::vector<AudioSource>& list) {
				list.clear();
				if (in.HasMember(key) && in[key].IsArray()) {
					for (auto& v : in[key].GetArray()) {
						AudioSource s;
						if (v.HasMember("name") && v["name"].IsString())   s.audioName = v["name"].GetString();
						if (v.HasMember("path") && v["path"].IsString())   s.audioPath = v["path"].GetString();
						if (v.HasMember("volume") && v["volume"].IsNumber()) s.volume = v["volume"].GetFloat();
						list.push_back(std::move(s));
					}
				}
				};
			readList("music", music);
			readList("sfx", sfx);

			currentMusicIndex = -1; currentMusicChannelId = -1;
		}

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

		template <typename Alloc>
		void Serialize(rapidjson::Value& out, Alloc& alloc) const {
			out.SetObject();
			out.AddMember("soundName", rapidjson::Value(soundName.c_str(), alloc), alloc);
			out.AddMember("eventName", rapidjson::Value(eventName.c_str(), alloc), alloc);
			out.AddMember("is3D", is3D, alloc);
			out.AddMember("isLooping", isLooping, alloc);
			out.AddMember("isStreaming", isStreaming, alloc);
			out.AddMember("volume", volume, alloc);
			out.AddMember("followTransform", followTransform, alloc);
			out.AddMember("minDistance", minDistance, alloc);
			out.AddMember("maxDistance", maxDistance, alloc);

			rapidjson::Value params(rapidjson::kObjectType);

			for (const auto& kv : eventParameters) {
				// key
				rapidjson::Value key;
				key.SetString(kv.first.c_str(),
					static_cast<rapidjson::SizeType>(kv.first.size()),
					alloc);

				// value (float/double)
				rapidjson::Value val;
				val.SetFloat(kv.second);              // or: val.SetDouble(static_cast<double>(kv.second));

				// Add (moves key and val into the object)
				params.AddMember(key, val, alloc);
			}

			out.AddMember(rapidjson::StringRef("eventParameters"), params, alloc);

		}
		void Deserialize(const rapidjson::Value& in) {
			if (in.HasMember("soundName") && in["soundName"].IsString()) soundName = in["soundName"].GetString();
			if (in.HasMember("eventName") && in["eventName"].IsString()) eventName = in["eventName"].GetString();
			if (in.HasMember("is3D")) is3D = in["is3D"].GetBool();
			if (in.HasMember("isLooping")) isLooping = in["isLooping"].GetBool();
			if (in.HasMember("isStreaming")) isStreaming = in["isStreaming"].GetBool();
			if (in.HasMember("volume")) volume = in["volume"].GetFloat();
			if (in.HasMember("followTransform")) followTransform = in["followTransform"].GetBool();
			if (in.HasMember("minDistance")) minDistance = in["minDistance"].GetFloat();
			if (in.HasMember("maxDistance")) maxDistance = in["maxDistance"].GetFloat();
			eventParameters.clear();
			if (in.HasMember("eventParameters") && in["eventParameters"].IsObject()) {
				for (auto it = in["eventParameters"].MemberBegin(); it != in["eventParameters"].MemberEnd(); ++it)
					if (it->value.IsNumber()) eventParameters[it->name.GetString()] = it->value.GetFloat();
			}
			channelId = -1; isPlaying = false; shouldPlay = shouldStop = false;
		}

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

		template <typename Alloc>
		void Serialize(rapidjson::Value& out, Alloc& alloc) const {
			out.SetObject();
			out.AddMember("velocity", Vec3ToJson(velocity, alloc), alloc);
			out.AddMember("lifetime", lifetime, alloc);
			out.AddMember("age", age, alloc);
			out.AddMember("colour", Vec4ToJson(colour, alloc), alloc);
			out.AddMember("size", size, alloc);
		}
		void Deserialize(const rapidjson::Value& in) {
			if (in.HasMember("velocity")) velocity = JsonToVec3(in["velocity"]);
			if (in.HasMember("lifetime")) lifetime = in["lifetime"].GetFloat();
			if (in.HasMember("age"))      age = in["age"].GetFloat();
			if (in.HasMember("colour") && in["colour"].IsArray() && in["colour"].Size() == 4)
				colour = Vec4(in["colour"][0].GetFloat(), in["colour"][1].GetFloat(), in["colour"][2].GetFloat(), in["colour"][3].GetFloat());
			if (in.HasMember("size"))     size = in["size"].GetFloat();
		}

	};

	/*!***********************************************************************
	 \brief
	  Hierarchy component structure for parent-child relationships.
	*************************************************************************/
	struct HierarchyComponent
	{
		static constexpr EntityID INVALID_PARENT = 0;

		EntityID parent = INVALID_PARENT;        // Parent entity ID
		std::vector<EntityID> children;         // List of child entity IDs
		int depth = 0;                          // Depth in hierarchy (root = 0)
		bool isDirty = false;                   // Flag for transform updates

		// Optional: Cache world transform for performance
		Mtx44 worldTransform{ 1.0f };             // Cached world transform
		bool worldTransformDirty = true;        // Separate flag for world transform cache

		// Constructors
		HierarchyComponent() = default;

		explicit HierarchyComponent(EntityID parentId)
			: parent(parentId), depth(0), isDirty(true), worldTransformDirty(true)
		{
		}
	};
	/*!***********************************************************************
	 \brief
	 Enum for Physic component.
	*************************************************************************/
	enum class PhysicsBodyType
	{
		Rigid,
		Trigger
	};
	enum class ShapeType { Box, Sphere, Capsule, CustomMesh/*, Compound*/,Total };

	/*!***********************************************************************
	 \brief
	 Physic component structure.
	*************************************************************************/
	struct PhysicComponent
	{
		JPH::BodyID bodyID{ JPH::BodyID::cInvalidBodyID };
		PhysicsBodyType bodyType{ PhysicsBodyType::Rigid };
		JPH::EMotionType motionType{ JPH::EMotionType::Static };
		float mass{ 0.0f };
		ShapeType shapeType{ ShapeType::Box };
		std::vector<Ermine::Vec3> customMeshVertices;   // For custom mesh

		PhysicComponent() = default;

		PhysicComponent(
			PhysicsBodyType type,
			JPH::EMotionType motion,
			float m = 0.0f,
			ShapeType shape = ShapeType::Box)
			: bodyType(type), motionType(motion), mass(m), shapeType(shape)
		{}

		template <typename Alloc>
		void Serialize(rapidjson::Value& out, Alloc& alloc) const {
			out.SetObject();
			const char* body = (bodyType == PhysicsBodyType::Trigger ? "Trigger" : "Rigid");
			out.AddMember("bodyType", rapidjson::Value(body, alloc), alloc);

			const char* motion = "Static";
			if (motionType == JPH::EMotionType::Kinematic) motion = "Kinematic";
			else if (motionType == JPH::EMotionType::Dynamic) motion = "Dynamic";
			out.AddMember("motionType", rapidjson::Value(motion, alloc), alloc);

			out.AddMember("mass", mass, alloc);

			static const char* shapeNames[] = { "Box", "Sphere", "Capsule", "CustomMesh" };
			out.AddMember("shapeType", rapidjson::Value(shapeNames[(int)shapeType], alloc), alloc);

			if (shapeType == ShapeType::CustomMesh) {
				rapidjson::Value verts(rapidjson::kArrayType);
				verts.Reserve(static_cast<rapidjson::SizeType>(customMeshVertices.size()), alloc);
				for (const auto& v : customMeshVertices) {
					rapidjson::Value arr(rapidjson::kArrayType);
					arr.PushBack(v.x, alloc).PushBack(v.y, alloc).PushBack(v.z, alloc);
					verts.PushBack(arr, alloc);
				}
				out.AddMember("customVertices", verts, alloc);
			}
		}

		void Deserialize(const rapidjson::Value& in) {
			if (in.HasMember("bodyType") && in["bodyType"].IsString())
				bodyType = (std::strcmp(in["bodyType"].GetString(), "Trigger") == 0) ? PhysicsBodyType::Trigger : PhysicsBodyType::Rigid;

			if (in.HasMember("motionType") && in["motionType"].IsString()) {
				auto s = in["motionType"].GetString();
				if (std::strcmp(s, "Kinematic") == 0) motionType = JPH::EMotionType::Kinematic;
				else if (std::strcmp(s, "Dynamic") == 0) motionType = JPH::EMotionType::Dynamic;
				else motionType = JPH::EMotionType::Static;
			}

			if (in.HasMember("mass") && in["mass"].IsNumber())
				mass = in["mass"].GetFloat();

			if (in.HasMember("shapeType") && in["shapeType"].IsString()) {
				auto s = in["shapeType"].GetString();
				if (std::strcmp(s, "Box") == 0) shapeType = ShapeType::Box;
				else if (std::strcmp(s, "Sphere") == 0) shapeType = ShapeType::Sphere;
				else if (std::strcmp(s, "Capsule") == 0) shapeType = ShapeType::Capsule;
				else shapeType = ShapeType::CustomMesh;
			}

			customMeshVertices.clear();
			if (shapeType == ShapeType::CustomMesh && in.HasMember("customVertices") && in["customVertices"].IsArray()) {
				for (auto& v : in["customVertices"].GetArray()) {
					if (v.IsArray() && v.Size() == 3)
						customMeshVertices.emplace_back(v[0].GetFloat(), v[1].GetFloat(), v[2].GetFloat());
				}
			}

			//bodyID = JPH::BodyID::cInvalidBodyID; // rebuilt by your physics system on scene init
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

		template <typename Alloc>
		void Serialize(rapidjson::Value& out, Alloc& alloc) const {
			const std::string& model_name = m_model->GetName();
			out.SetObject();

			rapidjson::Value modelVal;
			modelVal.SetString(model_name.c_str(),
				static_cast<rapidjson::SizeType>(model_name.size()),
				alloc);  // required for strings

			out.AddMember("model", modelVal, alloc);
		}


		void Deserialize(const rapidjson::Value& in) {
			if (in.HasMember("model") && in["model"].IsString()) {
				const char* name = in["model"].GetString();

				if (!m_model) {
					m_model = AssetManager::GetInstance().GetModel("../Resources/Models/" + std::string(name));
				}

				m_model->LoadModel(std::string("../Resources/Models/") + name);
			}
		}
	};

	/*!***********************************************************************
	\brief
	 Animation component structure.
	*************************************************************************/
	struct AnimationComponent
	{
		std::shared_ptr<graphics::Animator> m_animator;

		template <typename Alloc>
		void Serialize(rapidjson::Value& out, Alloc& alloc) const {
			const std::string& model_name = m_animator->GetModel()->GetName();
			out.SetObject();

			rapidjson::Value modelVal;
			modelVal.SetString(model_name.c_str(),
				static_cast<rapidjson::SizeType>(model_name.size()),
				alloc);  // required for strings

			out.AddMember("model", modelVal, alloc);
		}


		void Deserialize(const rapidjson::Value& in) {
			if (!in.IsObject()) return;

			if (in.HasMember("model") && in["model"].IsString()) {
				std::string modelName = in["model"].GetString();

				// Reload the model from assets
				auto model = AssetManager::GetInstance().GetModel("../Resources/Models/" + modelName);
				if (model) {
					const aiScene* scene = model->GetAssimpScene();
					if (scene && scene->mNumAnimations > 0) {
						m_animator = std::make_shared<graphics::Animator>(model);
						m_animator->LoadAnimations(scene);

						m_animator->PlayAnimation(0, true);
					}
				}
			}
		}
	};
}
