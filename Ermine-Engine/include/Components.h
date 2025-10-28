/* Start Header ************************************************************************/
/*!
\file       Components.h
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong\@digipen.edu (45%)
\co-author  Jeremy Lim Ting Jie, jeremytingjie.lim, 2301370, jeremytingjie.lim\@digipen.edu (10%)
\co-author  Ridhwan (5%)
\co-author  WEE HONG RU Curtis, h.wee, 2301266, h.wee\@digipen.edu (40%)
\date       Jan 24, 2025
\brief      Updated components with modular material system

Copyright (C) 2025 DigiPen Institute of Technology.
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
#include "Model.h"
#include "shadow_config.h"
#include "Animator.h"
#include "AssetManager.h"
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/Body.h>
#include "Guid.h"

#include "xcore/my_properties.h"
#include "xproperty.h"
#include "sprop/property_sprop.h"

#include "FSMNode.h"

namespace xprop_utils
{
	// Convert any xproperty-reflected object to JSON
	template<typename T, typename Alloc>
	void SerializeToJson(const T& obj, rapidjson::Value& out, Alloc& alloc)
	{
		out.SetObject();
		xproperty::settings::context ctx{};
		xproperty::sprop::container bag;
		xproperty::sprop::collector collect(obj, bag, ctx, true);

		for (auto& p : bag.m_Properties)
		{
			std::string key = p.m_Path.substr(p.m_Path.find_last_of('/') + 1);
			rapidjson::Value keyVal;
			keyVal.SetString(key.c_str(), (rapidjson::SizeType)key.size(), alloc);

			const auto guid = p.m_Value.getTypeGuid();

			// Serialize by GUID type
			if (guid == xproperty::settings::var_type<std::string>::guid_v)
			{
				const std::string& s = p.m_Value.get<std::string>();
				rapidjson::Value val;
				val.SetString(s.c_str(), (rapidjson::SizeType)s.size(), alloc);
				out.AddMember(keyVal, val, alloc);
			}
			else if (guid == xproperty::settings::var_type<bool>::guid_v)
				out.AddMember(keyVal, p.m_Value.get<bool>(), alloc);
			else if (guid == xproperty::settings::var_type<float>::guid_v)
				out.AddMember(keyVal, p.m_Value.get<float>(), alloc);
			else if (guid == xproperty::settings::var_type<int>::guid_v)
				out.AddMember(keyVal, p.m_Value.get<int>(), alloc);
			else if (guid == xproperty::settings::var_type<Ermine::Vec3>::guid_v)
				out.AddMember(keyVal, Vec3ToJson(p.m_Value.get<Ermine::Vec3>(), alloc), alloc);
			else if (guid == xproperty::settings::var_type<Ermine::Quaternion>::guid_v)
				out.AddMember(keyVal, QuatToJson(p.m_Value.get<Ermine::Quaternion>(), alloc), alloc);
		}
	}

	// Deserialize back into any reflected type
	template<typename T>
	void DeserializeFromJson(T& obj, const rapidjson::Value& in)
	{
		if (!in.IsObject()) return;
		xproperty::settings::context ctx{};
		xproperty::sprop::container bag;
		xproperty::sprop::collector collect(obj, bag, ctx, true);
		std::string err;

		for (auto& p : bag.m_Properties)
		{
			std::string key = p.m_Path.substr(p.m_Path.find_last_of('/') + 1);
			if (!in.HasMember(key.c_str())) continue;
			const auto& v = in[key.c_str()];
			const auto guid = p.m_Value.getTypeGuid();

			if (guid == xproperty::settings::var_type<std::string>::guid_v && v.IsString())
				p.m_Value.set<std::string>(v.GetString());
			else if (guid == xproperty::settings::var_type<bool>::guid_v && v.IsBool())
				p.m_Value.set<bool>(v.GetBool());
			else if (guid == xproperty::settings::var_type<float>::guid_v && v.IsNumber())
				p.m_Value.set<float>(v.GetFloat());
			else if (guid == xproperty::settings::var_type<int>::guid_v && v.IsInt())
				p.m_Value.set<int>(v.GetInt());
			else if (guid == xproperty::settings::var_type<Ermine::Vec3>::guid_v && v.IsArray() && v.Size() == 3)
				p.m_Value.set<Ermine::Vec3>(Ermine::Vec3(v[0].GetFloat(), v[1].GetFloat(), v[2].GetFloat()));
			else if (guid == xproperty::settings::var_type<Ermine::Quaternion>::guid_v && v.IsArray() && v.Size() == 4)
				p.m_Value.set<Ermine::Quaternion>(Ermine::Quaternion(v[0].GetFloat(), v[1].GetFloat(), v[2].GetFloat(), v[3].GetFloat()));

			xproperty::sprop::setProperty(err, obj, p, ctx);
		}
	}
}

namespace Ermine
{
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

	inline Quaternion JsonToVec4(const rapidjson::Value& arr) {
		if (arr.IsArray() && arr.Size() == 4) {
			return Quaternion(arr[0].GetFloat(), arr[1].GetFloat(), arr[2].GetFloat(), arr[3].GetFloat());
		}
		return Quaternion(0.f, 0.f, 0.f, 0.f); // default fallback
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

	struct IDComponent
	{
		Guid guid{};

		IDComponent() = default;
		explicit IDComponent(Guid g) : guid(g) {}

		template<typename Alloc>
		void Serialize(rapidjson::Value& out, Alloc& alloc) const {
			out.SetObject();
			out.AddMember("guid", rapidjson::Value(guid.ToString().c_str(), alloc), alloc);
		}
		void Deserialize(const rapidjson::Value& in) {
			if (in.HasMember("guid") && in["guid"].IsString()) {
				guid = Guid::FromString(in["guid"].GetString());
			}
		}
	};

	/*!***********************************************************************
	\brief
	 Transform component structure.
	*************************************************************************/
	struct Transform
	{
		//Mtx44 transform_matrix{ 1.0f }; // Local transform matrix
		Vec3 position;
		Quaternion rotation; // Euler angles in degrees
		Vec3 scale;
		bool isDirty{ true };

		explicit Transform(const Vec3& pos = Vec3(), const Quaternion& rot = Quaternion(), const Vec3& scl = Vec3(1.f, 1.f, 1.f))
			: position(pos), rotation(rot), scale(scl)
		{
		}

		// Helper method to build local transform matrix
		Mtx44 GetLocalMatrix() const
		{
			Mtx44 translation, rotation_mtx, scale_mtx;
			Mtx44Identity(translation);
			Mtx44Identity(rotation_mtx);
			Mtx44Identity(scale_mtx);

			// Set translation - this overwrites the translation part of identity matrix
			translation.m03 = position.x;
			translation.m13 = position.y;
			translation.m23 = position.z;

			// Set rotation from quaternion
			Mtx44SetFromQuaternion(rotation_mtx, rotation);

			// Set scale - this overwrites the scale part of identity matrix
			scale_mtx.m00 = scale.x;
			scale_mtx.m11 = scale.y;
			scale_mtx.m22 = scale.z;

			return translation * rotation_mtx * scale_mtx;
		}

		template<typename Alloc>
		void Serialize(rapidjson::Value& out, Alloc& alloc) const {
			xprop_utils::SerializeToJson(*this, out, alloc);
		}

		void Deserialize(const rapidjson::Value& in) {
			xprop_utils::DeserializeFromJson(*this, in);
		}

		XPROPERTY_DEF(
			"Transform", Transform,
			xproperty::obj_member<"position", &Transform::position>,
			xproperty::obj_member<"rotation", &Transform::rotation>,
			xproperty::obj_member<"scale", &Transform::scale>
		);
	};

	//XPROPERTY_REG(Transform);

	/*!***********************************************************************
	\brief
	 GlobalTransform component - WORLD TRANSFORM FOR RENDERING.
	*************************************************************************/
	struct GlobalTransform
	{
		Mtx44 worldMatrix{ 1.0f };  // World transform matrix used by renderer
		bool isDirty{ true };

		GlobalTransform() = default;

		explicit GlobalTransform(const Mtx44& matrix) : worldMatrix(matrix), isDirty(false) {}

		// Extract world position from matrix
		Vec3 GetWorldPosition() const
		{
			return Vec3(worldMatrix.m03, worldMatrix.m13, worldMatrix.m23);
		}

		// Extract world rotation from matrix
		Quaternion GetWorldRotation() const
		{
			// Remove translation and scale to get rotation matrix
			Mtx44 rotMatrix = worldMatrix;
			rotMatrix.m03 = 0.0f; rotMatrix.m13 = 0.0f; rotMatrix.m23 = 0.0f; rotMatrix.m33 = 1.0f;

			// Remove scale
			Vec3 xAxis(rotMatrix.m00, rotMatrix.m10, rotMatrix.m20);
			Vec3 yAxis(rotMatrix.m01, rotMatrix.m11, rotMatrix.m21);
			Vec3 zAxis(rotMatrix.m02, rotMatrix.m12, rotMatrix.m22);

			float xLen = sqrtf(xAxis.x * xAxis.x + xAxis.y * xAxis.y + xAxis.z * xAxis.z);
			float yLen = sqrtf(yAxis.x * yAxis.x + yAxis.y * yAxis.y + yAxis.z * yAxis.z);
			float zLen = sqrtf(zAxis.x * zAxis.x + zAxis.y * zAxis.y + zAxis.z * zAxis.z);

			if (xLen > 0.0f) { xAxis.x /= xLen; xAxis.y /= xLen; xAxis.z /= xLen; }
			if (yLen > 0.0f) { yAxis.x /= yLen; yAxis.y /= yLen; yAxis.z /= yLen; }
			if (zLen > 0.0f) { zAxis.x /= zLen; zAxis.y /= zLen; zAxis.z /= zLen; }

			rotMatrix.m00 = xAxis.x; rotMatrix.m10 = xAxis.y; rotMatrix.m20 = xAxis.z;
			rotMatrix.m01 = yAxis.x; rotMatrix.m11 = yAxis.y; rotMatrix.m21 = yAxis.z;
			rotMatrix.m02 = zAxis.x; rotMatrix.m12 = zAxis.y; rotMatrix.m22 = zAxis.z;

			return Mtx44GetQuaternion(rotMatrix);
		}

		// Extract world scale from matrix
		Vec3 GetWorldScale() const
		{
			Vec3 xAxis(worldMatrix.m00, worldMatrix.m10, worldMatrix.m20);
			Vec3 yAxis(worldMatrix.m01, worldMatrix.m11, worldMatrix.m21);
			Vec3 zAxis(worldMatrix.m02, worldMatrix.m12, worldMatrix.m22);

			return Vec3(
				sqrtf(xAxis.x * xAxis.x + xAxis.y * xAxis.y + xAxis.z * xAxis.z),
				sqrtf(yAxis.x * yAxis.x + yAxis.y * yAxis.y + yAxis.z * yAxis.z),
				sqrtf(zAxis.x * zAxis.x + zAxis.y * zAxis.y + zAxis.z * zAxis.z)
			);
		}

		template<typename Alloc>
		void Serialize(rapidjson::Value& out, Alloc& alloc) const {
			out.SetObject();
			// Don't serialize the matrix - it gets recalculated from hierarchy
		}

		void Deserialize(const rapidjson::Value& in) {
			// Don't deserialize the matrix - it gets recalculated from hierarchy
			isDirty = true;
		}

		XPROPERTY_DEF(
			"GlobalTransform", GlobalTransform,
			xproperty::obj_member<"isDirty", &GlobalTransform::isDirty>
		);
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
			xprop_utils::SerializeToJson(*this, out, alloc);
		}

		void Deserialize(const rapidjson::Value& in) {
			xprop_utils::DeserializeFromJson(*this, in);
		}

		XPROPERTY_DEF(
			"ObjectMetaData", ObjectMetaData,
			xproperty::obj_member<"name", &ObjectMetaData::name>,
			xproperty::obj_member<"tag", &ObjectMetaData::tag>,
			xproperty::obj_member<"selfActive", &ObjectMetaData::selfActive>
		)
	};

	struct ScriptFieldValue
	{
		enum class Kind { Float = 0, Int = 1, Bool = 2, String = 3, Vector3 = 4, Quaternion = 5 };

		using Value = std::variant<float, int, bool, std::string, Vec3, Quaternion>;

		Kind  kind = Kind::Float;
		Value value = 0.0f;

		static ScriptFieldValue MakeFloat(float v)
		{
			ScriptFieldValue r;
			r.kind = Kind::Float;
			r.value = v;
			return r;
		}

		static ScriptFieldValue MakeInt(int v)
		{
			ScriptFieldValue r;
			r.kind = Kind::Int;
			r.value = v;
			return r;
		}

		static ScriptFieldValue MakeBool(bool v)
		{
			ScriptFieldValue r;
			r.kind = Kind::Bool;
			r.value = v;
			return r;
		}

		static ScriptFieldValue MakeString(std::string v)
		{
			ScriptFieldValue r;
			r.kind = Kind::String;
			r.value = std::move(v);
			return r;
		}

		static ScriptFieldValue MakeVec3(const Vec3& v)
		{
			ScriptFieldValue r;
			r.kind = Kind::Vector3;
			r.value = v;
			return r;
		}

		static ScriptFieldValue MakeQuat(const Quaternion& v)
		{
			ScriptFieldValue r;
			r.kind = Kind::Quaternion;
			r.value = v;
			return r;
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

		std::unordered_map<std::string, ScriptFieldValue> m_fields;

		Script() = default;
		explicit Script(std::string className, EntityID id) : m_className(std::move(className))
		{
			auto sc = std::make_unique<scripting::ScriptClass>(scripting::ScriptClass("", m_className));
			m_instance = std::make_unique<scripting::ScriptInstance>(std::move(sc), id);
		}

		Script(const Script& other) : m_className(other.m_className), m_fields(other.m_fields)
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
				m_fields = other.m_fields;
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
			m_instance(std::move(other.m_instance)), m_fields(std::move(other.m_fields))
		{
		}

		Script& operator=(Script&& other) noexcept
		{
			if (this != &other)
			{
				m_className = std::move(other.m_className);
				m_instance = std::move(other.m_instance);
				m_fields = std::move(other.m_fields);
			}
			return *this;
		}

		template <typename Alloc>
		void Serialize(rapidjson::Value& out, Alloc& alloc) const {
			out.SetObject();
			out.AddMember("class", rapidjson::Value(m_className.c_str(), alloc), alloc);
			out.AddMember("enabled", m_enabled, alloc);

			rapidjson::Value fields(rapidjson::kObjectType);
			for (const auto& kv : m_fields)
			{
				rapidjson::Value fld(rapidjson::kObjectType);
				fld.AddMember("t", static_cast<int>(kv.second.kind), alloc);
				try
				{
					switch (kv.second.kind)
					{
					case ScriptFieldValue::Kind::Float:
						fld.AddMember("v", std::get<float>(kv.second.value), alloc); break;
					case ScriptFieldValue::Kind::Int:
						fld.AddMember("v", std::get<int>(kv.second.value), alloc); break;
					case ScriptFieldValue::Kind::Bool:
						fld.AddMember("v", std::get<bool>(kv.second.value), alloc); break;
					case ScriptFieldValue::Kind::String:
						fld.AddMember("v", rapidjson::Value(std::get<std::string>(kv.second.value), alloc), alloc); break;
					case ScriptFieldValue::Kind::Vector3:
						fld.AddMember("v", Vec3ToJson(std::get<Vec3>(kv.second.value), alloc), alloc); break;
					case ScriptFieldValue::Kind::Quaternion:
						fld.AddMember("v", QuatToJson(std::get<Quaternion>(kv.second.value), alloc), alloc); break;
					default: break;
					}
				} catch (const std::bad_variant_access& ex)
				{
					// Skip invalid variant access
					EE_CORE_WARN(ex.what());
					continue;
				}

				fields.AddMember(rapidjson::Value(kv.first.c_str(), alloc), fld, alloc);
			}
			out.AddMember("fields", fields, alloc);
		}

		void Deserialize(const rapidjson::Value& in) {
			if (in.HasMember("class") && in["class"].IsString()) m_className = in["class"].GetString();
			if (in.HasMember("enabled") && in["enabled"].IsBool()) m_enabled = in["enabled"].GetBool();
			// Note: re-create ScriptInstance when attaching to entity (needs EntityID)

			m_fields.clear();
			if (in.HasMember("fields") && in["fields"].IsObject())
			{
				for (auto it = in["fields"].MemberBegin(); it != in["fields"].MemberEnd(); ++it)
				{
					const std::string name = it->name.GetString();
					const auto& fld = it->value;
					if (!fld.IsObject() || !fld.HasMember("t") || !fld.HasMember("v")) continue;

					const int t = fld["t"].GetInt();
					switch (static_cast<ScriptFieldValue::Kind>(t))
					{
					case ScriptFieldValue::Kind::Float:
						if (fld["v"].IsNumber()) m_fields[name] = ScriptFieldValue::MakeFloat(fld["v"].GetFloat());
						break;
					case ScriptFieldValue::Kind::Int:
						if (fld["v"].IsInt()) m_fields[name] = ScriptFieldValue::MakeInt(fld["v"].GetInt());
						break;
					case ScriptFieldValue::Kind::Bool:
						if (fld["v"].IsBool()) m_fields[name] = ScriptFieldValue::MakeBool(fld["v"].GetBool());
						break;
					case ScriptFieldValue::Kind::String:
						if (fld["v"].IsString()) m_fields[name] = ScriptFieldValue::MakeString(fld["v"].GetString());
						break;
					case ScriptFieldValue::Kind::Vector3:
						if (fld["v"].IsArray()) m_fields[name] = ScriptFieldValue::MakeVec3(JsonToVec3(fld["v"]));
						break;
					case ScriptFieldValue::Kind::Quaternion:
						if (fld["v"].IsArray()) m_fields[name] = ScriptFieldValue::MakeQuat(JsonToVec4(fld["v"]));
						break;
					default: break;
					}
				}
			}
		}

		XPROPERTY_DEF(
			"Script", Script,
			xproperty::obj_member<"class", &Script::m_className>,
			xproperty::obj_member<"enabled", &Script::m_enabled>
		)
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

	struct MeshPrimitiveDesc {
		std::string type;
		Vec3        size{ 1,1,1 };

		XPROPERTY_DEF(
			"MeshPrimitiveDesc", MeshPrimitiveDesc,
			xproperty::obj_member<"type", &MeshPrimitiveDesc::type>,
			xproperty::obj_member<"size", &MeshPrimitiveDesc::size>
		)
	};

	struct MeshAssetDesc {
		std::string meshName;

		XPROPERTY_DEF(
			"MeshAssetDesc", MeshAssetDesc,
			xproperty::obj_member<"meshName", &MeshAssetDesc::meshName>
		)
	};

	enum class MeshKind { None, Primitive, Asset };
}

namespace xproperty::settings {
	template<>
	struct var_type<Ermine::MeshKind> : var_defaults<"MeshKind", Ermine::MeshKind>
	{
		// Antlion xproperty: enum_item is constructed FROM THE ENUM (not ints)
		inline static constexpr std::array enum_list_v{
			enum_item{"None",      Ermine::MeshKind::None},
			enum_item{"Primitive", Ermine::MeshKind::Primitive},
			enum_item{"Asset",     Ermine::MeshKind::Asset},
		};
	};
} // namespace xproperty::settings

namespace Ermine
{
	/*!***********************************************************************
	\brief
	 Mesh structure
	*************************************************************************/
	struct Mesh
	{
		std::shared_ptr<graphics::VertexArray> vertex_array;
		std::shared_ptr<graphics::VertexBuffer> vertex_buffer;
		std::shared_ptr<graphics::IndexBuffer> index_buffer;

		MeshKind        kind = MeshKind::None;
		MeshPrimitiveDesc primitive;
		MeshAssetDesc     asset;

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
			case MeshKind::Primitive: {
				out.AddMember("kind", "Primitive", alloc);
				rapidjson::Value p(rapidjson::kObjectType);
				p.AddMember("type", rapidjson::Value(primitive.type.c_str(), alloc), alloc);
				p.AddMember("size", Vec3ToJson(primitive.size, alloc), alloc);
				out.AddMember("primitive", p, alloc);
				break;
			}
			case MeshKind::Asset: {
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
			if (!in.HasMember("kind")) { kind = MeshKind::None; return; }
			const auto& k = in["kind"];
			if (k.IsString()) {
				if (strcmp(k.GetString(), "Primitive") == 0) {
					kind = MeshKind::Primitive;
					const auto& p = in["primitive"];
					if (p.HasMember("type") && p["type"].IsString()) primitive.type = p["type"].GetString();
					if (p.HasMember("size")) primitive.size = JsonToVec3(p["size"]);

					RebuildPrimitive();
					// extend for other primitives
				}

				else if (strcmp(k.GetString(), "Asset") == 0) {
					kind = MeshKind::Asset;
					const auto& a = in["asset"];
					if (a.HasMember("meshName") && a["meshName"].IsString()) asset.meshName = a["meshName"].GetString();
					// Rebuild from asset if you have a mesh loader separate from Model
				}
				else {
					kind = MeshKind::None;
				}
			}
		}

		// XPROPERTY_DEF
		XPROPERTY_DEF(
			"Mesh", Mesh,
			xproperty::obj_member<"kind", &Mesh::kind>,
			xproperty::obj_member<"primitive", &Mesh::primitive>,
			xproperty::obj_member<"asset", &Mesh::asset>
		)
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
		 * @param texture The texture to associate with the material (optional). If valid, it is set as the albedo map only.
		 */
		Material(const std::shared_ptr<graphics::Shader>& shader, const std::shared_ptr<graphics::Texture>& texture)
		{
			m_material = std::make_shared<graphics::Material>(shader);

			// Only set texture and flags if texture is explicitly provided and valid
			if (texture && texture->IsValid())
			{
				m_material->SetTexture("materialAlbedoMap", texture);
				m_material->SetBool("materialHasAlbedoMap", true);
			}

			// Set default PBR values without any texture assumptions
			m_material->LoadTemplate(graphics::MaterialTemplates::PBR_WHITE());
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

			auto writeVec4 = [&](const char* name) {
				if (auto p = m_material->GetParameter(name); p && p->floatValues.size() >= 4) {
					rapidjson::Value a(rapidjson::kArrayType);
					a.PushBack(p->floatValues[0], alloc)
						.PushBack(p->floatValues[1], alloc)
						.PushBack(p->floatValues[2], alloc)
						.PushBack(p->floatValues[3], alloc);
					params.AddMember(rapidjson::StringRef(name), a, alloc);

					// Also emit explicit alpha & transparency fields for compatibility
					const float alpha = p->floatValues[3];
					rapidjson::Value alphaVal; alphaVal.SetFloat(alpha);
					params.AddMember("materialAlpha", alphaVal, alloc);

					rapidjson::Value tVal; tVal.SetFloat(1.0f - alpha);
					params.AddMember("materialTransparency", tVal, alloc);
					return true;
				}
				return false;
				};

			// Core PBR parameters (prefer RGBA if available; fall back to RGB)
			bool wroteRGBA = writeVec4("materialAlbedo");
			if (!wroteRGBA) {
				// Legacy RGB path
				writeVec3("materialAlbedo");
				// If an explicit alpha was authored as separate fields, preserve them too
				writeFloat("materialAlpha");
				writeFloat("materialTransparency");
			}
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

				// --- DESERIALIZE: accept vec4/vec3 + (alpha or transparency) and normalize to vec4 ---
				auto readVec4 = [&](const char* name, const char* compatName = nullptr) {
					if (p.HasMember(name) && p[name].IsArray() && p[name].Size() == 4) {
						Vec4 v{ p[name][0].GetFloat(), p[name][1].GetFloat(), p[name][2].GetFloat(), p[name][3].GetFloat() };
						m_material->SetVec4(name, v);
						if (compatName) m_material->SetVec4(compatName, v);
						// Keep explicit alpha/transparency mirrors in params for editor UIs
						m_material->SetFloat("materialAlpha", v.w);
						m_material->SetFloat("materialTransparency", 1.0f - v.w);
						return true;
					}
					return false;
					};

				auto readAlphaOrTransparency = [&]() -> std::optional<float> {
					// Prefer alpha if present; else compute from transparency
					if (p.HasMember("materialAlpha") && p["materialAlpha"].IsNumber())
						return p["materialAlpha"].GetFloat();
					if (p.HasMember("materialTransparency") && p["materialTransparency"].IsNumber()) {
						float tr = p["materialTransparency"].GetFloat();
						return 1.0f - tr;
					}
					return std::nullopt;
					};

				// Core PBR parameters (albedo first)
				bool readRGBA = readVec4("materialAlbedo", "material.albedo");
				if (!readRGBA) {
					// Legacy RGB load
					readVec3("materialAlbedo", "material.albedo");
					// If we only have RGB, try to augment with alpha/transparency
					if (auto alphaOpt = readAlphaOrTransparency()) {
						// Build a Vec4 from the currently set RGB (or defaults if absent)
						Vec3 rgb{ 0.8f, 0.8f, 0.8f };
						if (const auto* cur = m_material->GetParameter("materialAlbedo")) {
							if (cur->floatValues.size() >= 3) {
								rgb = Vec3(cur->floatValues[0], cur->floatValues[1], cur->floatValues[2]);
							}
						}
						const float a = std::clamp(*alphaOpt, 0.0f, 1.0f);
						m_material->SetVec4("materialAlbedo", Vec4(rgb.x, rgb.y, rgb.z, a));
						m_material->SetVec4("material.albedo", Vec4(rgb.x, rgb.y, rgb.z, a));
						m_material->SetFloat("materialAlpha", a);
						m_material->SetFloat("materialTransparency", 1.0f - a);
					}
				}
				else {
					// If RGBA was present, we've already mirrored alpha/transparency above
				}

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

		XPROPERTY_DEF(
			"Material", Material,
			// authoring template name
			xproperty::obj_member<"template", &Material::materialTemplate>,

			// cached parameters
			xproperty::obj_member<"hasAlbedo", &Material::hasAlbedo>,
			xproperty::obj_member<"albedo", &Material::cacheAlbedo>,

			xproperty::obj_member<"hasRough", &Material::hasRough>,
			xproperty::obj_member<"roughness", &Material::cacheRoughness>,

			xproperty::obj_member<"hasMetal", &Material::hasMetal>,
			xproperty::obj_member<"metallic", &Material::cacheMetallic>,

			xproperty::obj_member<"hasEmiss", &Material::hasEmiss>,
			xproperty::obj_member<"emissive", &Material::cacheEmissive>,
			xproperty::obj_member<"emissiveIntensity", &Material::cacheEmissiveIntensity>
		)
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
}

namespace xproperty::settings {
	template<>
	struct var_type<Ermine::LightType> : var_defaults<"LightType", Ermine::LightType>
	{
		// antlion: enum_item takes the enum, not integers
		inline static constexpr std::array enum_list_v{
			enum_item{"Point",       Ermine::LightType::POINT},
			enum_item{"Directional", Ermine::LightType::DIRECTIONAL},
			enum_item{"Spot",        Ermine::LightType::SPOT},
		};
	};
}

namespace Ermine
{
	/*!***********************************************************************
	\brief
	 Light structure
	*************************************************************************/
	struct Light {
		Vec3 color{};
		float intensity{};
		LightType type{};
		bool castsShadows{ false };
		glm::mat4 lightSpaceMatrices[NUM_CASCADES]{}; // For shadow mapping
		int startOffset{ 0 }; // For UBO indexing
		float innerAngle{ -1.0f }; // For spotlights
		float outerAngle{ -1.0f }; // For spotlights
		float radius{ 3.0f }; // For point lights/spotlights
		float splitDepths[NUM_CASCADES]{};

		Light() : color(1.0f, 1.0f, 1.0f),
			intensity(1.0f),
			type(LightType::POINT),
			lightSpaceMatrices{},
			splitDepths{}
		{
		}

		Light(const Vec3& col, float intens, LightType t) :
			color(col), intensity(intens), type(t),
			lightSpaceMatrices{},
			splitDepths{}
		{
		}

		Light(const Vec3& col, float intens, LightType t, bool shadows) :
			color(col), intensity(intens), type(t), castsShadows(shadows),
			lightSpaceMatrices{},
			splitDepths{}
		{
		}

		Light(const Vec3& col, float intens, LightType t, bool shadows, float inner, float outer, float rad = 1.0f) :
			color(col), intensity(intens), type(t), castsShadows(shadows), innerAngle(inner), outerAngle(outer), radius(rad),
			lightSpaceMatrices{},
			splitDepths{}
		{
		}

		template<typename Alloc>
		void Serialize(rapidjson::Value& out, Alloc& alloc) const {
			xprop_utils::SerializeToJson(*this, out, alloc);
		}

		void Deserialize(const rapidjson::Value& in) {
			xprop_utils::DeserializeFromJson(*this, in);
		}

		XPROPERTY_DEF(
			"Light", Light,
			xproperty::obj_member<"color", &Light::color>,
			xproperty::obj_member<"intensity", &Light::intensity>,
			xproperty::obj_member<"type", &Light::type>,
			xproperty::obj_member<"castsShadows", &Light::castsShadows>,
			xproperty::obj_member<"innerAngle", &Light::innerAngle>,  // used for spot
			xproperty::obj_member<"outerAngle", &Light::outerAngle>,  // used for spot
			xproperty::obj_member<"radius", &Light::radius>       // used for point/spot
		)
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

		// Reflect AudioSource (name, path, volume)
		XPROPERTY_DEF(
			"AudioSource", AudioSource,
			xproperty::obj_member<"name", &AudioSource::audioName>,
			xproperty::obj_member<"path", &AudioSource::audioPath>,
			xproperty::obj_member<"volume", &AudioSource::volume>
		)
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

		void UpdateMusicSource(int index, const std::string& name, const std::string& path)
		{
			if (index >= 0 && index < static_cast<int>(music.size()))
			{
				// Stop current music if we're updating the currently playing track
				if (currentMusicIndex == index && currentMusicChannelId != -1)
				{
					CAudioEngine::StopChannel(currentMusicChannelId);
					currentMusicChannelId = -1;
				}

				// Update the music source
				music[index].audioName = name;
				music[index].audioPath = path;

				// Reload the sound with new path
				try
				{
					CAudioEngine::LoadSound(path, false, true, true); // Background music is typically looped and streamed
				}
				catch (const std::exception& e)
				{
					(void)e;
					// Handle loading error if needed
					UNREFERENCED_PARAMETER(e);
				}
			}
		}

		// Update an existing SFX source
		void UpdateSFXSource(int index, const std::string& name, const std::string& path)
		{
			if (index >= 0 && index < static_cast<int>(sfx.size()))
			{
				// Update the SFX source
				sfx[index].audioName = name;
				sfx[index].audioPath = path;

				// Reload the sound with new path
				try
				{
					CAudioEngine::LoadSound(path, false, false, false); // SFX typically not looped/streamed
				}
				catch (const std::exception& e)
				{
					(void)e;
					// Handle loading error if needed
					UNREFERENCED_PARAMETER(e);
				}
			}
		}

		// Remove a music source
		void RemoveMusicSource(int index)
		{
			if (index >= 0 && index < static_cast<int>(music.size()))
			{
				// Stop current music if we're removing the currently playing track
				if (currentMusicIndex == index && currentMusicChannelId != -1)
				{
					CAudioEngine::StopChannel(currentMusicChannelId);
					currentMusicChannelId = -1;
					currentMusicIndex = -1;
				}
				else if (currentMusicIndex > index)
				{
					// Adjust current music index if a track before it was removed
					currentMusicIndex--;
				}

				// Remove the music source
				music.erase(music.begin() + index);
			}
		}

		// Remove an SFX source
		void RemoveSFXSource(int index)
		{
			if (index >= 0 && index < static_cast<int>(sfx.size()))
			{
				// Remove the SFX source
				sfx.erase(sfx.begin() + index);
			}
		}

		// Get music source by index (for safe access)
		const AudioSource* GetMusicSource(int index) const
		{
			if (index >= 0 && index < static_cast<int>(music.size()))
			{
				return &music[index];
			}
			return nullptr;
		}

		// Get SFX source by index (for safe access)
		const AudioSource* GetSFXSource(int index) const
		{
			if (index >= 0 && index < static_cast<int>(sfx.size()))
			{
				return &sfx[index];
			}
			return nullptr;
		}

		// Find music index by name
		int FindMusicIndex(const std::string& name) const
		{
			for (size_t i = 0; i < music.size(); ++i)
			{
				if (music[i].audioName == name)
				{
					return static_cast<int>(i);
				}
			}
			return -1;
		}

		// Find SFX index by name
		int FindSFXIndex(const std::string& name) const
		{
			for (size_t i = 0; i < sfx.size(); ++i)
			{
				if (sfx[i].audioName == name)
				{
					return static_cast<int>(i);
				}
			}
			return -1;
		}

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

		XPROPERTY_DEF(
			"GlobalAudioComponent", GlobalAudioComponent,
			xproperty::obj_member<"masterVolume", &GlobalAudioComponent::masterVolume>,
			xproperty::obj_member<"musicVolume", &GlobalAudioComponent::musicVolume>,
			xproperty::obj_member<"sfxVolume", &GlobalAudioComponent::sfxVolume>
		)
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

		template<typename Alloc>
		void Serialize(rapidjson::Value& out, Alloc& alloc) const {
			xprop_utils::SerializeToJson(*this, out, alloc);
		}

		void Deserialize(const rapidjson::Value& in) {
			xprop_utils::DeserializeFromJson(*this, in);
		}

		XPROPERTY_DEF(
			"AudioComponent", AudioComponent,
			xproperty::obj_member<"soundName", &AudioComponent::soundName>,
			xproperty::obj_member<"eventName", &AudioComponent::eventName>,
			xproperty::obj_member<"is3D", &AudioComponent::is3D>,
			xproperty::obj_member<"isLooping", &AudioComponent::isLooping>,
			xproperty::obj_member<"isStreaming", &AudioComponent::isStreaming>,
			xproperty::obj_member<"volume", &AudioComponent::volume>,
			xproperty::obj_member<"followTransform", &AudioComponent::followTransform>,
			xproperty::obj_member<"minDistance", &AudioComponent::minDistance>,
			xproperty::obj_member<"maxDistance", &AudioComponent::maxDistance>
		)
	};

	/*!***********************************************************************
	 \brief
	 Particle component structure.
	*************************************************************************/
	/*struct Particle
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
	};*/

	struct ParticleEmitter
	{
		bool active = true;
		float emissionRate = 10.0f;  // particles per second
		float particleLifetime = 2.0f;
		float particleSize = 0.2f;
		Vec3 velocity = { 0, 2, 0 };
		Vec3 position = { 0.0f, 0.0f, 0.0f };
		std::string textureName = "../Resources/Textures/greybox_light_solid.png";
		float timeAccumulator = 0.0f;

		template<typename Alloc>
		void Serialize(rapidjson::Value& out, Alloc& alloc) const {
			xprop_utils::SerializeToJson(*this, out, alloc);
		}

		void Deserialize(const rapidjson::Value& in) {
			xprop_utils::DeserializeFromJson(*this, in);
		}
		
		XPROPERTY_DEF(
			"ParticleEmitterComponent", ParticleEmitter,
			xproperty::obj_member<"active", &ParticleEmitter::active>,
			xproperty::obj_member<"emissionRate", &ParticleEmitter::emissionRate>,
			xproperty::obj_member<"particleLifetime", &ParticleEmitter::particleLifetime>,
			xproperty::obj_member<"particleSize", &ParticleEmitter::particleSize>,
			xproperty::obj_member<"velocity", &ParticleEmitter::velocity>,
			xproperty::obj_member<"textureName", &ParticleEmitter::textureName>
		);
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

		template <typename Alloc>
		void Serialize(rapidjson::Value& out, Alloc& alloc) const {
			out.SetObject();
			UNREFERENCED_PARAMETER(alloc);
		}
		void Deserialize(const rapidjson::Value& in) {
			(void)in;
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
	enum class ShapeType { Box, Sphere, Capsule, CustomMesh/*, Compound*/, Total };
}

namespace xproperty::settings {
	template<> struct var_type<Ermine::PhysicsBodyType> : var_defaults<"PhysicsBodyType", Ermine::PhysicsBodyType> {
		inline static constexpr std::array enum_list_v{
			enum_item{"Rigid",   Ermine::PhysicsBodyType::Rigid},
			enum_item{"Trigger", Ermine::PhysicsBodyType::Trigger},
		};
	};

	template<> struct var_type<JPH::EMotionType> : var_defaults<"JPH_EMotionType", JPH::EMotionType> {
		inline static constexpr std::array enum_list_v{
			enum_item{"Static",    JPH::EMotionType::Static},
			enum_item{"Kinematic", JPH::EMotionType::Kinematic},
			enum_item{"Dynamic",   JPH::EMotionType::Dynamic},
		};
	};

	template<> struct var_type<Ermine::ShapeType> : var_defaults<"ShapeType", Ermine::ShapeType> {
		inline static constexpr std::array enum_list_v{
			enum_item{"Box",        Ermine::ShapeType::Box},
			enum_item{"Sphere",     Ermine::ShapeType::Sphere},
			enum_item{"Capsule",    Ermine::ShapeType::Capsule},
			enum_item{"CustomMesh", Ermine::ShapeType::CustomMesh},
		};
	};
}

namespace Ermine
{
	/*!***********************************************************************
	 \brief
	 Physic component structure.
	*************************************************************************/
	struct PhysicComponent
	{
		JPH::BodyID bodyID{ JPH::BodyID::cInvalidBodyID };
		JPH::Body* body{ nullptr };
		PhysicsBodyType bodyType{ PhysicsBodyType::Rigid };
		JPH::EMotionType motionType{ JPH::EMotionType::Static };
		float mass{ 0.0f };
		ShapeType shapeType{ ShapeType::Box };
		std::vector<glm::vec3> customMeshVertices;   // For custom mesh
		JPH::RefConst<JPH::Shape> shapeRef;

		PhysicComponent() = default;

		PhysicComponent(
			PhysicsBodyType type,
			JPH::EMotionType motion,
			float m = 0.0f,
			ShapeType shape = ShapeType::Box)
			: bodyType(type), motionType(motion), mass(m), shapeType(shape)
		{
		}

		template<typename Alloc>
		void Serialize(rapidjson::Value& out, Alloc& alloc) const {
			xprop_utils::SerializeToJson(*this, out, alloc);
		}

		void Deserialize(const rapidjson::Value& in) {
			xprop_utils::DeserializeFromJson(*this, in);
		}

		XPROPERTY_DEF(
			"PhysicComponent", PhysicComponent,
			xproperty::obj_member<"bodyType", &PhysicComponent::bodyType>,
			xproperty::obj_member<"motionType", &PhysicComponent::motionType>,
			xproperty::obj_member<"mass", &PhysicComponent::mass>,
			xproperty::obj_member<"shapeType", &PhysicComponent::shapeType>
		)
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

		//XPROPERTY_DEF(
		//	"ModelComponent", ModelComponent,
		//	xproperty::obj_member<"modelName", &ModelComponent::modelName>
		//)
	};

	/*!***********************************************************************
	\brief
	 Animation component structure.
	*************************************************************************/
	struct AnimationComponent
	{
		std::shared_ptr<graphics::Animator> m_animator;

		AnimationComponent() = default;
		explicit AnimationComponent(const std::shared_ptr<graphics::Model>& model) : m_animator(std::make_shared<graphics::Animator>(model)) {}

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
					}
				}
			}
		}

		//XPROPERTY_DEF(
		//	"AnimationComponent", AnimationComponent,
		//	xproperty::obj_member<"animModelName", &AnimationComponent::animModelName>
		//)
	};

	/*!***********************************************************************
	\brief
	 State Machine component structure.
	*************************************************************************/
	class StateManager; // forward declaration
	struct StateMachine
	{
		StateManager* manager = nullptr;
		ScriptNode* m_CurrentScript = nullptr;
		ScriptNode* m_PreviousScript = nullptr;

		std::deque<std::shared_ptr<ScriptNode>> m_Nodes;
		std::vector<std::pair<int, int>> m_Links;
		std::unordered_map<ScriptNode*, ScriptNode*> scriptTransitions;
	public:
		// For script start
		void Init(EntityID entity)
		{
			ScriptNode* startScript = nullptr;

			// Find node marked as start
			for (auto& n : m_Nodes)
			{
				if (n->isStartNode)
				{
					startScript = n.get();
					break;
				}
			}

			// Fallback to first node if none is marked
			if (!startScript && !m_Nodes.empty())
			{
				startScript = m_Nodes.front().get();
				//EE_CORE_INFO("FSM: No start node marked, using first node '%s' (id=%d)", startScript->name.c_str(), startScript->id);
			}

			// Assign and initialize
			m_CurrentScript = startScript;

			if (m_CurrentScript)
			{
				m_CurrentScript->CreateInstance(entity);
				m_CurrentScript->OnEnter();
				EE_CORE_INFO("FSM: Initialized with start node '%s' (id=%d)",
					m_CurrentScript->name.c_str(), m_CurrentScript->id);
			}
			else
			{
				EE_CORE_WARN("FSM: Init() called but no valid start node found for entity %d!", entity);
			}
		}
		/*!***********************************************************************
		\brief
		   Update the current state of an entity.
		*************************************************************************/
		void Update(EntityID entity, float dt)
		{
			// Ensure current script exists and is valid
			if (!m_CurrentScript ||
				std::find_if(m_Nodes.begin(), m_Nodes.end(),
					[&](const std::shared_ptr<ScriptNode>& n) { return n.get() == m_CurrentScript; }) == m_Nodes.end())
			{
				//EE_CORE_WARN("FSM: Current script for entity %d invalid or deleted. Attempting recovery...", entity);

				// Try to find start node
				m_CurrentScript = nullptr;
				for (auto& n : m_Nodes)
				{
					if (n->isStartNode)
					{
						m_CurrentScript = n.get();
						//EE_CORE_INFO("FSM: Reset to start node '%s' (id=%d)", n->name.c_str(), n->id);
						break;
					}
				}

				// Fallback to first node if no start node
				if (!m_CurrentScript && !m_Nodes.empty())
				{
					m_CurrentScript = m_Nodes.front().get();
					//EE_CORE_INFO("FSM: Fallback to first node '%s' (id=%d)", m_CurrentScript->name.c_str(), m_CurrentScript->id);
				}

				// Initialize recovered node
				if (m_CurrentScript)
				{
					m_CurrentScript->CreateInstance(entity);
					m_CurrentScript->OnEnter();
				}
				else
				{
					//EE_CORE_WARN("FSM: No valid nodes found; skipping update.");
					return;
				}
			}

			// Run script update logic (C# handles transitions now)
			if (m_CurrentScript && m_CurrentScript->instance)
				m_CurrentScript->OnUpdate();
		}
	};
}