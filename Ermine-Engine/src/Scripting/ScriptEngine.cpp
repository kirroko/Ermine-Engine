/* Start Header ************************************************************************/
/*!
\file       ScriptEngine.cpp
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong\@digipen.edu
\date       09/08/2025
\brief      This files contains the declaration for Mono Script Engine
			This is the main interface for the scripting engine, it should be used to initialize and shutdown the engine.
			It also provides functions to execute scripts, call functions and get/set variables.
			The script engine is based on Mono, a cross-platform implementation of the .NET framework.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "PreCompile.h"
#include "ScriptEngine.h"

#include "ECS.h"
#include "Components.h"
#include "FrameController.h"
#include "Input.h"
#include "Logger.h"

void Ermine::scripting::ScriptEngine::InitMono(const std::string& assembly_path)
{
	EE_CORE_TRACE("Init Mono...");
	mono_set_dirs("mono/lib", "mono/etc");
	mono_set_assemblies_path("mono/lib/4.5");

	m_coreDomain = mono_jit_init_version("ErmineCore", "v4.0.30319");
	if (!m_coreDomain)
	{
		EE_CORE_ERROR("Failed to initialize Mono Core Domain");
		return;
	}
	char gdn[] = "ErmineGame";
	m_gameDomain = mono_domain_create_appdomain(gdn, nullptr);
	mono_domain_set(m_gameDomain, true);

	// Loading engine API assembly
	m_apiAsm = LoadCSharpAssembly(assembly_path);
	if (!m_apiAsm)
	{
		EE_CORE_ERROR("Failed to load API assembly: {0}", assembly_path);
		return;
	}
	PrintAssemblyTypes(m_apiAsm);

	RegisterInternalCalls();
}

void Ermine::scripting::ScriptEngine::Shutdown()
{
	EE_CORE_TRACE("Shutdown Mono...");

	if (!m_coreDomain)
		return;

	mono_jit_cleanup(m_coreDomain);
	m_coreDomain = nullptr;
}

void Ermine::scripting::ScriptEngine::PrintAssemblyTypes(MonoAssembly* assembly)
{
	MonoImage* image = mono_assembly_get_image(assembly);
	const MonoTableInfo* typeDefinitionsTable = mono_image_get_table_info(image, MONO_TABLE_TYPEDEF);
	int32_t numTypes = mono_table_info_get_rows(typeDefinitionsTable);

	for (int32_t i = 0; i < numTypes; i++)
	{
		uint32_t cols[MONO_TYPEDEF_SIZE];
		mono_metadata_decode_row(typeDefinitionsTable, i, cols, MONO_TYPEDEF_SIZE);

		const char* nameSpace = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAMESPACE]);
		const char* name = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAME]);

		EE_CORE_TRACE("{0}.{1}", nameSpace, name);
	}
}

char* Ermine::scripting::ScriptEngine::ReadBytes(const std::string& filepath, uint32_t* outSize)
{
	std::ifstream stream(filepath, std::ios::binary | std::ios::ate);

	if (!stream)
	{
		// Failed to open the file
		EE_CORE_WARN("Failed to open the file {0}", filepath);
		return nullptr;
	}

	std::streampos end = stream.tellg();
	stream.seekg(0, std::ios::beg);
	long long size = end - stream.tellg();

	if (size == 0)
	{
		// File is empty
		EE_CORE_WARN("Files is empty! {0}", filepath);
		return nullptr;
	}

	char* buffer = new char[size];
	stream.read((char*)buffer, size);
	stream.close();

	*outSize = (uint32_t)size;
	return buffer;
}

MonoAssembly* Ermine::scripting::ScriptEngine::LoadCSharpAssembly(const std::string& assemblyPath)
{
	uint32_t fileSize = 0;
	char* fileData = ReadBytes(assemblyPath, &fileSize);

	// NOTE: We can't use this image for anything other than loading the assembly because this image doesn't have a reference to the assembly
	MonoImageOpenStatus status;
	MonoImage* image = mono_image_open_from_data_full(fileData, fileSize, 1, &status, 0);

	if (status != MONO_IMAGE_OK)
	{
		const char* errorMessage = mono_image_strerror(status);
		EE_CORE_ERROR("{0}", errorMessage);
		return nullptr;
	}

	MonoAssembly* assembly = mono_assembly_load_from_full(image, assemblyPath.c_str(), &status, 0);
	mono_image_close(image);

	// Don't forget to free the file data
	delete[] fileData;

	return assembly;
}

MonoAssembly* Ermine::scripting::ScriptEngine::LoadGameAssembly(const std::string& assemblyPath)
{
	m_gameAssemblyPath = assemblyPath;

	if (m_gameAsm) // If we already have a game assembly loaded, close it first
	{
		mono_assembly_close(m_gameAsm);
		m_gameAsm = nullptr;
	}

	m_gameAsm = LoadCSharpAssembly(assemblyPath);
	if (!m_gameAsm)
	{
		EE_CORE_ERROR("Failed to load game assembly: {0}", assemblyPath);
		return nullptr;
	}

	EE_CORE_TRACE("Game assembly loaded: {0}", assemblyPath);
	PrintAssemblyTypes(m_gameAsm);
	return m_gameAsm;
}

void Ermine::scripting::ScriptEngine::ReloadGameAssembly()
{
	if (m_gameAssemblyPath.empty())
	{
		EE_CORE_WARN("Cannot reload game assembly: No path specified");
		return;
	}

	mono_domain_set(m_gameDomain, false);

	LoadGameAssembly(m_gameAssemblyPath);
}

namespace
{
	// Caching
	static MonoImage* s_APIImage = nullptr;
	static MonoClass* s_GameObjectClass = nullptr;
	static MonoClass* s_TransformClass = nullptr;
	static MonoClass* s_MonoBehaviourClass = nullptr;
	static MonoClass* s_ComponentClass = nullptr;

	void ToTempUTF8(MonoString* str, std::string& out)
	{
		if (!str)
		{
			out.clear();
			//return out.c_str();
			return;
		}
		char* raw = mono_string_to_utf8(str);
		out = raw ? raw : "";
		if (raw)
			mono_free(raw);
		/*return out.c_str();*/
	}

	Ermine::EntityID GetEntityIDFromManaged(MonoObject* obj)
	{
		if (!obj) return 0;
		MonoClass* klass = mono_object_get_class(obj);
		if (MonoClassField* field = mono_class_get_field_from_name(klass, "EntityID"))
		{
			Ermine::EntityID id = 0;
			mono_field_get_value(obj, field, &id);
			return id;
		}
		return 0;
	}

	void SetEntityIDOnManaged(MonoObject* obj, Ermine::EntityID id)
	{
		if (!obj) return;
		MonoClass* klass = mono_object_get_class(obj);
		if (MonoClassField* field = mono_class_get_field_from_name(klass, "EntityID"))
			mono_field_set_value(obj, field, &id);
	}

	MonoClass* GetAPIClass(const char* nameSpace, const char* name)
	{
		if (!s_APIImage)
			return nullptr;
		return mono_class_from_name(s_APIImage, nameSpace, name);
	}

	bool IsSubclassOf(MonoClass* klass, MonoClass* parentKlass)
	{
		if (!klass || !parentKlass)
			return false;
		MonoClass* current = klass;
		while (current)
		{
			if (current == parentKlass) return true;
			current = mono_class_get_parent(current);
		}
		return false;
	}

	MonoObject* CreateManagedGameObjectWrapper(Ermine::EntityID id)
	{
		if (!s_GameObjectClass)
			return nullptr;
		MonoObject* obj = mono_object_new(mono_domain_get(), s_GameObjectClass); // Creating Gameobject object on managed side
		// Don't call mono_runtime_object_init (its ctor would create a new native entity)
		SetEntityIDOnManaged(obj, id); // ID is from the native side, it when script was created
		return obj;
	}

	MonoObject* CreateManagedTransformWrapper(Ermine::EntityID id)
	{
		if (!s_TransformClass)
			return nullptr;
		MonoObject* obj = mono_object_new(mono_domain_get(), s_TransformClass);
		mono_runtime_object_init(obj);
		SetEntityIDOnManaged(obj, id);
		return obj;
	}


	void SetComponentGameObject(MonoObject* componentObj, Ermine::EntityID id)
	{
		if (!componentObj || !s_ComponentClass || !s_GameObjectClass)
			return;
		MonoMethod* setGO = mono_class_get_method_from_name(s_ComponentClass, "set_gameObject", 1);
		if (!setGO)
			return;

		MonoObject* goWrapper = CreateManagedGameObjectWrapper(id);
		void* args[1] = { goWrapper };
		mono_runtime_invoke(setGO, componentObj, args, nullptr);
	}

	struct ManagedVector3 { float x, y, z; };
	ManagedVector3 ToManaged(const Ermine::Vec3& v) { return { v.x, v.y, v.z }; }
	Ermine::Vec3 ToNative(const ManagedVector3& v) { return { v.x, v.y, v.z }; }

	Ermine::Transform* GetTransformFromManaged(MonoObject* thisObj)
	{
		using namespace Ermine;
		EntityID id = GetEntityIDFromManaged(thisObj);
		if (id == 0 || !ECS::GetInstance().IsEntityValid(id) || !ECS::GetInstance().HasComponent<Transform>(id))
		{
			EE_CORE_WARN("Cannot get Transform for {0} ; either not valid or doesn't have transform component!", id);
			return nullptr;
		}
		return &ECS::GetInstance().GetComponent<Transform>(id);
	}

#pragma region Trasnform ICalls
	ManagedVector3 icall_transform_get_position(MonoObject* thisObj)
	{
		if (auto* t = GetTransformFromManaged(thisObj))
			return ToManaged(t->position);
		EE_CORE_WARN("Transform for {0} failed to get unmanaged position", GetEntityIDFromManaged(thisObj));
		return { 0,0,0 };
	}

	ManagedVector3 icall_transform_get_rotation(MonoObject* thisObj)
	{
		if (auto* t = GetTransformFromManaged(thisObj))
			return ToManaged(t->rotation);
		EE_CORE_WARN("Transform for {0} failed to get unmanaged rotation", GetEntityIDFromManaged(thisObj));
		return { 0,0,0 };
	}

	ManagedVector3 icall_transform_get_scale(MonoObject* thisObj)
	{
		if (auto* t = GetTransformFromManaged(thisObj))
			return ToManaged(t->scale);
		EE_CORE_WARN("Transform for {0} failed to get unmanaged scale", GetEntityIDFromManaged(thisObj));
		return { 1,1,1 };
	}

	void icall_transform_set_position(MonoObject* thisObj, ManagedVector3 value)
	{
		if (auto* t = GetTransformFromManaged(thisObj))
			t->position = ToNative(value);
	}

	void icall_transform_set_rotation(MonoObject* thisObj, ManagedVector3 value)
	{
		if (auto* t = GetTransformFromManaged(thisObj))
			t->rotation = ToNative(value);
	}

	void icall_transform_set_scale(MonoObject* thisObj, ManagedVector3 value)
	{
		if (auto* t = GetTransformFromManaged(thisObj))
			t->scale = ToNative(value);
	}
#pragma endregion

#pragma region Time ICalls
	float icall_time_get_deltatime() { return Ermine::FrameController::GetDeltaTime(); }
	float icall_time_get_fixeddeltatime() { return Ermine::FrameController::GetFixedDeltaTime(); }
#pragma endregion

#pragma region Input ICalls
	bool icall_input_getkey(int key)
	{
		return Ermine::Input::IsKeyPressed(key);
	}

	bool icall_input_getkeydown(int key)
	{
		return Ermine::Input::IsKeyDown(key);
	}

	bool icall_input_getkeyup(int key)
	{
		return Ermine::Input::IsKeyReleased(key);
	}
#pragma endregion

#pragma region Debug ICalls
	void icall_debug_log_info(MonoString* message)
	{
		std::string temp;
		ToTempUTF8(message, temp);
		EE_CORE_INFO("{}", temp);
	}

	void icall_debug_log_warning(MonoString* message)
	{
		std::string temp;
		ToTempUTF8(message, temp);
		EE_CORE_WARN("{}", temp);
	}

	void icall_debug_log_error(MonoString* message)
	{
		std::string temp;
		ToTempUTF8(message, temp);
		EE_CORE_ERROR("{}", temp);
	}
#pragma endregion

#pragma region Object ICalls
	MonoString* icall_object_get_name(MonoObject* thisObj)
	{
		using namespace Ermine;
		EntityID id = GetEntityIDFromManaged(thisObj);
		if (id == 0 || !ECS::GetInstance().IsEntityValid(id) || !ECS::GetInstance().HasComponent<ObjectMetaData>(id))
		{
			EE_CORE_WARN("Cannot get name for {0}; either not valid or doesn't have object meta data!", id);
			return mono_string_new(mono_domain_get(), "GameObject");
		}
		auto& meta = ECS::GetInstance().GetComponent<ObjectMetaData>(id);
		return mono_string_new(mono_domain_get(), meta.name.c_str());
	}

	void icall_object_set_name(MonoObject* thisObj, MonoString* value)
	{
		using namespace Ermine;
		EntityID id = GetEntityIDFromManaged(thisObj);
		if (id == 0 || !ECS::GetInstance().IsEntityValid(id) || !ECS::GetInstance().HasComponent<ObjectMetaData>(id))
		{
			EE_CORE_WARN("Cannot get name for {0}; either not valid or doesn't have object meta data!", id);
			return;
		}

		std::string temp;
		ToTempUTF8(value, temp);
		auto& meta = ECS::GetInstance().GetComponent<ObjectMetaData>(id);
		meta.name = std::move(temp);
	}
#pragma endregion

#pragma region Component ICalls
	MonoString* icall_component_get_tag(MonoObject* thisObj)
	{
		using namespace Ermine;
		EntityID id = GetEntityIDFromManaged(thisObj);
		if (id == 0 || !ECS::GetInstance().IsEntityValid(id) || !ECS::GetInstance().HasComponent<ObjectMetaData>(id))
		{
			EE_CORE_WARN("Cannot get tag for {0} ; either not valid or doesn't have object meta data!", id);
			return mono_string_new(mono_domain_get(), "unnamedTag");
		}
		auto& meta = ECS::GetInstance().GetComponent<ObjectMetaData>(id);
		return mono_string_new(mono_domain_get(), meta.tag.c_str());
	}

	void icall_component_set_tag(MonoObject* thisObj, MonoString* value)
	{
		using namespace Ermine;
		EntityID id = GetEntityIDFromManaged(thisObj);
		if (id == 0 || !ECS::GetInstance().IsEntityValid(id) || !ECS::GetInstance().HasComponent<ObjectMetaData>(id))
		{
			EE_CORE_WARN("Cannot set tag for {0} ; either not valid or doesn't have object meta data!", id);
			return;
		}

		std::string temp;
		ToTempUTF8(value, temp);
		auto& meta = ECS::GetInstance().GetComponent<ObjectMetaData>(id);
		meta.tag = std::move(temp);
	}
#pragma endregion

#pragma region GameObject ICalls
	void icall_gameobject_creategameobject(MonoObject* self, MonoString* name)
	{
		EE_CORE_WARN("GameObject created!");
		using namespace Ermine;
		if (!self)
			return;

		std::string name_;
		ToTempUTF8(name, name_);
		if (name_.empty()) name_ = "GameObject";

		EntityID id = ECS::GetInstance().CreateEntity();
		ECS::GetInstance().AddComponent(id, Transform());

		ObjectMetaData meta;
		meta.name = name_;
		meta.tag = "Untagged";
		meta.selfActive = true;
		ECS::GetInstance().AddComponent(id, std::move(meta));

		SetEntityIDOnManaged(self, id); // Set the entityID on the managed object (Which is 'GameObject'). Which is primarily what managed object only needs to know
	}

	MonoObject* icall_gameobject_get_transform(MonoObject* self)
	{
		using namespace Ermine;
		EntityID id = GetEntityIDFromManaged(self);
		if (id == 0 || !ECS::GetInstance().IsEntityValid(id) || !ECS::GetInstance().HasComponent<Transform>(id))
			return nullptr;
		MonoObject* obj = CreateManagedGameObjectWrapper(id);
		SetComponentGameObject(obj, id);
		return obj;
	}

	bool icall_gameobject_get_active(MonoObject* self)
	{
		using namespace Ermine;
		EntityID id = GetEntityIDFromManaged(self);
		if (id == 0 || !ECS::GetInstance().IsEntityValid(id) || !ECS::GetInstance().HasComponent<ObjectMetaData>(id))
			return false;
		return ECS::GetInstance().GetComponent<ObjectMetaData>(id).selfActive;
	}

	void icall_gameobject_set_active(MonoObject* self, bool value)
	{
		using namespace Ermine;
		EntityID id = GetEntityIDFromManaged(self);
		if (id == 0 || !ECS::GetInstance().IsEntityValid(id) || !ECS::GetInstance().HasComponent<ObjectMetaData>(id))
			return;
		ECS::GetInstance().GetComponent<ObjectMetaData>(id).selfActive = value;
	}

	MonoObject* icall_gameobject_add_component(MonoObject* self, MonoReflectionType* reflType)
	{
		using namespace Ermine;
		if (!self || !reflType) return nullptr;

		EntityID id = GetEntityIDFromManaged(self);
		if (id == 0 || !ECS::GetInstance().IsEntityValid(id))
			return nullptr;

		MonoType* mtype = mono_reflection_type_get_type(reflType);
		MonoClass* klass = mono_class_from_mono_type(mtype);
		if (!klass) return nullptr;

		if (klass == s_TransformClass)
		{
			if (!ECS::GetInstance().HasComponent<Transform>(id))
				ECS::GetInstance().AddComponent(id, Transform());
			MonoObject* obj = CreateManagedTransformWrapper(id);
			SetComponentGameObject(obj, id);
			return obj;
		}

		// MonoBehaviour derived -> Script component
		if (IsSubclassOf(klass,s_MonoBehaviourClass))
		{
			const char* cname = mono_class_get_name(klass);
			EE_CORE_WARN("Is subclass of MonoBehaviour!");
			if (!ECS::GetInstance().HasComponent<Script>(id))
			{
				ECS::GetInstance().AddComponent(id, Script(std::string(cname), id));
			}
			else
			{
				ECS::GetInstance().RemoveComponent<Script>(id);
				ECS::GetInstance().AddComponent(id, Script(std::string(cname), id));
			}

			auto& scriptComp = ECS::GetInstance().GetComponent<Script>(id);
			if (scriptComp.m_instance && scriptComp.m_instance->object)
				SetComponentGameObject(scriptComp.m_instance->object, id);
			return scriptComp.m_instance ? scriptComp.m_instance->object : nullptr;
		}

		EE_CORE_WARN("AddComponent: Unsupported component type '{0}'", mono_class_get_name(klass));
		return nullptr;
	}

	MonoObject* icall_gameobject_get_component(MonoObject* self, MonoReflectionType* relfType)
	{
		using namespace Ermine;
		if (!self || !relfType) return nullptr;

		EntityID id = GetEntityIDFromManaged(self);
		if (id == 0 || !ECS::GetInstance().IsEntityValid(id))
			return nullptr;

		MonoType* mtype = mono_reflection_type_get_type(relfType);
		MonoClass* klass = mono_class_from_mono_type(mtype);
		if (!klass) return nullptr;

		if (klass == s_TransformClass)
		{
			if (!ECS::GetInstance().HasComponent<Transform>(id))
				return nullptr;
			MonoObject* obj = CreateManagedTransformWrapper(id);
			SetComponentGameObject(obj, id);
			return obj;
		}

		if (IsSubclassOf(klass, s_MonoBehaviourClass))
		{
			if (!ECS::GetInstance().HasComponent<Script>(id))
				return nullptr;
			auto& scriptComp = ECS::GetInstance().GetComponent<Script>(id);
			if (scriptComp.m_instance && scriptComp.m_instance->object)
				SetComponentGameObject(scriptComp.m_instance->object, id);
			return scriptComp.m_instance ? scriptComp.m_instance->object : nullptr;
		}

		return nullptr;
	}

	bool icall_gameobject_has_component(MonoObject* self, MonoReflectionType* relfType)
	{
		using namespace Ermine;
		if (!self || !relfType) return false;
		EntityID id = GetEntityIDFromManaged(self);
		if (id == 0 || !ECS::GetInstance().IsEntityValid(id))
			return false;

		MonoType* mtype = mono_reflection_type_get_type(relfType);
		MonoClass* klass = mono_class_from_mono_type(mtype);
		if (!klass) return false;

		if (klass == s_TransformClass)
			return ECS::GetInstance().HasComponent<Transform>(id);
		
		if (IsSubclassOf(klass, s_MonoBehaviourClass))
			return ECS::GetInstance().HasComponent<Script>(id);
		
		return false;
	}

	void icall_gameobject_remove_component(MonoObject* self, MonoReflectionType* relfType)
	{
		using namespace Ermine;
		if (!self || !relfType) return;
		EntityID id = GetEntityIDFromManaged(self);
		if (id == 0 || !ECS::GetInstance().IsEntityValid(id))
			return;
		MonoType* mtype = mono_reflection_type_get_type(relfType);
		MonoClass* klass = mono_class_from_mono_type(mtype);
		if (!klass) return;
		if (klass == s_TransformClass)
		{
			//if (ECS::GetInstance().HasComponent<Transform>(id))
			//	ECS::GetInstance().RemoveComponent<Transform>(id);
			EE_CORE_WARN("RemoveComponent<Transform> is not allowed!");
			return;
		}
		if (IsSubclassOf(klass, s_MonoBehaviourClass))
		{
			if (ECS::GetInstance().HasComponent<Script>(id))
				ECS::GetInstance().RemoveComponent<Script>(id);
		}
	}

	MonoObject* icall_gameobject_find_by_name(MonoString* name)
	{
		using namespace Ermine;
		std::string name_;
		ToTempUTF8(name, name_);
		if (name_.empty()) return nullptr;
		auto& ecs = ECS::GetInstance();
		for (unsigned long int i = 0; i < ecs.GetLivingEntityCount(); i++) // TODO: Optimize this later
		{
			EntityID id = i + 1; // Entity IDs start from 1
			if (!ecs.IsEntityValid(id) || !ecs.HasComponent<ObjectMetaData>(id))
				continue;
			auto& meta = ecs.GetComponent<ObjectMetaData>(id);
			if (meta.name == name_)
				return CreateManagedGameObjectWrapper(id);
		}
		return nullptr;
	}

	MonoObject* icall_gameobject_find_with_tag(MonoString* tag)
	{
		using namespace Ermine;
		std::string tag_;
		ToTempUTF8(tag, tag_);
		if (tag_.empty()) return nullptr;
		auto& ecs = ECS::GetInstance();
		for (unsigned long int i = 0; i < ecs.GetLivingEntityCount(); i++) // TODO: Optimize this later
		{
			EntityID id = i + 1; // Entity IDs start from 1
			if (!ecs.IsEntityValid(id) || !ecs.HasComponent<ObjectMetaData>(id))
				continue;
			auto& meta = ecs.GetComponent<ObjectMetaData>(id);
			if (meta.tag == tag_)
				return CreateManagedGameObjectWrapper(id);
		}
		return nullptr;
	}

	MonoArray* icall_gameobject_find_gameobjects_with_tag(MonoString* mtag)
	{
		EE_CORE_WARN("Not ready yet.");
		return nullptr;
	}

	MonoObject* icall_gameobject_instantiate(MonoObject* original)
	{
		using namespace Ermine;
		if (!original) return nullptr;
		EntityID srcID = GetEntityIDFromManaged(original);
		if (srcID == 0 || !ECS::GetInstance().IsEntityValid(srcID))
			return nullptr;

		EntityID dstID = ECS::GetInstance().CreateEntity();

		// TODO: Copy all components from srcID to dstID, It will grow over time with more components
		// Copy Transform
		if (ECS::GetInstance().HasComponent<Transform>(srcID))
		{
			auto t = ECS::GetInstance().GetComponent<Transform>(srcID);
			ECS::GetInstance().AddComponent(dstID, t);
		}
		else
			ECS::GetInstance().AddComponent(dstID, Transform());

		// Copy Meta
		if (ECS::GetInstance().HasComponent<ObjectMetaData>(srcID))
		{
			auto meta = ECS::GetInstance().GetComponent<ObjectMetaData>(srcID);
			meta.name += "(Clone)";
			ECS::GetInstance().AddComponent(dstID, meta);
		}
		else
			ECS::GetInstance().AddComponent(dstID, ObjectMetaData());

		// Copy Script?
		if (ECS::GetInstance().HasComponent<Script>(srcID))
		{
			auto& scriptSrc = ECS::GetInstance().GetComponent<Script>(srcID);
			ECS::GetInstance().AddComponent(dstID, Script(scriptSrc.m_className, dstID));
		}

		MonoObject* obj = CreateManagedGameObjectWrapper(dstID);
		SetComponentGameObject(obj, dstID);
		return obj;
	}

	void icall_gameobject_destroy(MonoObject* target)
	{
		using namespace Ermine;
		if (!target) return;
		EntityID id = GetEntityIDFromManaged(target);
		if (id == 0 || !ECS::GetInstance().IsEntityValid(id))
			return;
		ECS::GetInstance().DestroyEntity(id);
	}
#pragma endregion
}

namespace Ermine::scripting
{
	// For the forward declaration in ScriptInstance.h
	void NativeBindComponentGameObject(MonoObject* componentObj, EntityID id)
	{
		if (!componentObj || !s_ComponentClass || !s_GameObjectClass)
			return;

		MonoMethod* setGO = mono_class_get_method_from_name(s_ComponentClass, "set_gameObject", 1);
		if (!setGO)
			return;

		MonoObject* goWrapper = CreateManagedGameObjectWrapper(id);
		if (!goWrapper)
			return;

		void* args[1] = { goWrapper };
		MonoObject* exc = nullptr;
		mono_runtime_invoke(setGO, componentObj, args, &exc);
		if (exc)
		{
			MonoString* s = mono_object_to_string(exc, nullptr);
			char* utf8 = mono_string_to_utf8(s);
			EE_CORE_ERROR("NativeBindComponentGameObject exception: {}", utf8);
			if (utf8) mono_free(utf8);
		}
	}
}

void Ermine::scripting::ScriptEngine::RegisterInternalCalls()
{
	s_APIImage = mono_assembly_get_image(m_apiAsm);
	s_GameObjectClass = GetAPIClass("ErmineEngine", "GameObject");
	s_TransformClass = GetAPIClass("ErmineEngine", "Transform");
	s_ComponentClass = GetAPIClass("ErmineEngine", "Component");
	s_MonoBehaviourClass = GetAPIClass("ErmineEngine", "MonoBehaviour");

#pragma region Transform ICalls
	mono_add_internal_call("ErmineEngine.Transform::get_position", (const void*)icall_transform_get_position);
	mono_add_internal_call("ErmineEngine.Transform::get_rotation", (const void*)icall_transform_get_rotation);
	mono_add_internal_call("ErmineEngine.Transform::get_scale", (const void*)icall_transform_get_scale);
	mono_add_internal_call("ErmineEngine.Transform::set_position", (const void*)icall_transform_set_position);
	mono_add_internal_call("ErmineEngine.Transform::set_rotation", (const void*)icall_transform_set_rotation);
	mono_add_internal_call("ErmineEngine.Transform::set_scale", (const void*)icall_transform_set_scale);
#pragma endregion
	
#pragma region Time ICalls
	mono_add_internal_call("ErmineEngine.Time::get_deltaTime",	(const void*)icall_time_get_deltatime);
	mono_add_internal_call("ErmineEngine.Time::get_fixedDeltaTime", (const void*)icall_time_get_fixeddeltatime);
#pragma endregion

#pragma region Input ICalls
	mono_add_internal_call("ErmineEngine.Input::InternalGetKey", (const void*)icall_input_getkey);
	mono_add_internal_call("ErmineEngine.Input::InternalGetKeyDown", (const void*)icall_input_getkeydown);
	mono_add_internal_call("ErmineEngine.Input::InternalGetKeyUp", (const void*)icall_input_getkeyup);
#pragma endregion

#pragma region Debug ICalls
	mono_add_internal_call("ErmineEngine.Debug::LogInternal",			(const void*)icall_debug_log_info);
	mono_add_internal_call("ErmineEngine.Debug::LogWarningInternal",	(const void*)icall_debug_log_warning);
	mono_add_internal_call("ErmineEngine.Debug::LogErrorInternal",		(const void*)icall_debug_log_error);
#pragma endregion

#pragma region Object ICalls
	mono_add_internal_call("ErmineEngine.Object::get_name", (const void*)icall_object_get_name);
	mono_add_internal_call("ErmineEngine.Object::set_name", (const void*)icall_object_set_name);
#pragma endregion

#pragma region Component ICalls
	mono_add_internal_call("ErmineEngine.Component::get_tag", (const void*)icall_component_get_tag);
	mono_add_internal_call("ErmineEngine.Component::set_tag", (const void*)icall_component_set_tag);
#pragma endregion

#pragma region GameObject ICalls
	mono_add_internal_call("ErmineEngine.GameObject::Internal_CreateGameObject", (const void*)icall_gameobject_creategameobject);
	mono_add_internal_call("ErmineEngine.GameObject::Internal_GetTransform", (const void*)icall_gameobject_get_transform);
	mono_add_internal_call("ErmineEngine.GameObject::Internal_GetActive", (const void*)icall_gameobject_get_active);
	mono_add_internal_call("ErmineEngine.GameObject::Internal_SetActive", (const void*)icall_gameobject_set_active);
	mono_add_internal_call("ErmineEngine.GameObject::Internal_AddComponent", (const void*)icall_gameobject_add_component);
	mono_add_internal_call("ErmineEngine.GameObject::Internal_GetComponent", (const void*)icall_gameobject_get_component);
	mono_add_internal_call("ErmineEngine.GameObject::Internal_HasComponent", (const void*)icall_gameobject_has_component);
	mono_add_internal_call("ErmineEngine.GameObject::Internal_RemoveComponent", (const void*)icall_gameobject_remove_component);
	mono_add_internal_call("ErmineEngine.GameObject::Internal_FindByName", (const void*)icall_gameobject_find_by_name);
	mono_add_internal_call("ErmineEngine.GameObject::Internal_FindWithTag", (const void*)icall_gameobject_find_with_tag);
	mono_add_internal_call("ErmineEngine.GameObject::Internal_FindGameObjectsWithTag", (const void*)icall_gameobject_find_gameobjects_with_tag);
	mono_add_internal_call("ErmineEngine.GameObject::Internal_Instantiate", (const void*)icall_gameobject_instantiate);
	mono_add_internal_call("ErmineEngine.GameObject::Internal_Destroy", (const void*)icall_gameobject_destroy);
#pragma endregion
}
