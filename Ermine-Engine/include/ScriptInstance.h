/* Start Header ************************************************************************/
/*!
\file       ScriptInstance.h
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong\@digipen.edu
\date       07/08/2025
\brief      This file contains the detail of per-entity script

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/
#pragma once
#include "ScriptClass.h"

namespace Ermine::scripting
{
	void NativeBindComponentGameObject(MonoObject* componentObj, EntityID id);

	struct ScriptInstance
	{
		std::unique_ptr<ScriptClass> klass;
		MonoObject* object = nullptr;
		MonoGCHandle GCHandle = nullptr; // Mono GC handle for the script instance
		EntityID	entityID; // Entity ID this script instance is attached to

		bool m_enabled = false;

		explicit ScriptInstance(std::unique_ptr<ScriptClass> k, const EntityID eid) : klass(std::move(k)), entityID(eid)
		{
			EE_CORE_TRACE("ScriptInstance: Ctor");
			object = klass ? klass->Instantiate() : nullptr;
			if (!object)
			{
				EE_CORE_ERROR("ScriptInstance: failed to instantiate managed object");
				return;
			}
			GCHandle = mono_gchandle_new_v2(object, false);
			InjectEntityIfAvailable();
			NativeBindComponentGameObject(object, eid);
			Awake(); // called when an enabled script instance is being loaded.
		}

		~ScriptInstance()
		{
			if (m_enabled) OnDisable();
			Invoke(klass ? klass->MethodOnDestroy : nullptr);
			if (GCHandle)
			{
				mono_gchandle_free_v2(GCHandle);
				GCHandle = nullptr;
			}
			object = nullptr;
		}

		ScriptInstance(const ScriptInstance&) = delete;
		ScriptInstance& operator= (const ScriptInstance&) = delete;

		void Awake() { Invoke(klass ? klass->MethodAwake : nullptr); }
		void Start() { Invoke(klass ? klass->MethodStart : nullptr); }
		void Update() { Invoke(klass ? klass->MethodUpdate : nullptr); }
		void FixedUpdate() { Invoke(klass ? klass->MethodFixedUpdate : nullptr); }
		void OnEnable() { Invoke(klass ? klass->MethodOnEnable : nullptr); }
		void OnDisable() { Invoke(klass ? klass->MethodOnDisable : nullptr); }

		void OnCollisionEnter(EntityID other, bool isTrigger)
		{
			InvokeWithCollision(klass ? klass->MethodOnCollisionEnter : nullptr, other, isTrigger);
		}
		void OnCollisionExit(EntityID other, bool isTrigger)
		{
			InvokeWithCollision(klass ? klass->MethodOnCollisionExit : nullptr, other, isTrigger);
		}
		void OnCollisionStay(EntityID other, bool isTrigger)
		{
			InvokeWithCollision(klass ? klass->MethodOnCollisionStay : nullptr, other, isTrigger);
		}
		void OnTriggerEnter(EntityID other, bool isTrigger)
		{
			InvokeWithCollision(klass ? klass->MethodOnTriggerEnter : nullptr, other, isTrigger);
		}
		void OnTriggerExit(EntityID other, bool isTrigger)
		{
			InvokeWithCollision(klass ? klass->MethodOnTriggerExit : nullptr, other, isTrigger);
		}
		void OnTriggerStay(EntityID other, bool isTrigger)
		{
			InvokeWithCollision(klass ? klass->MethodOnTriggerStay : nullptr, other, isTrigger);
		}
		//void OnCollisionEnter() { Invoke(klass ? klass->MethodOnCollisionEnter : nullptr); }
		//void OnCollisionExit() { Invoke(klass ? klass->MethodOnCollisionExit : nullptr); }
		//void OnCollisionStay() { Invoke(klass ? klass->MethodOnCollisionStay : nullptr); }
		//void OnTriggerEnter() { Invoke(klass ? klass->MethodOnTriggerEnter : nullptr); }
		//void OnTriggerExit() { Invoke(klass ? klass->MethodOnTriggerExit : nullptr); }
		//void OnTriggerStay() { Invoke(klass ? klass->MethodOnTriggerStay : nullptr); }

		void SetEnabled(bool enabled)
		{
			if (enabled == m_enabled) return;
			m_enabled = enabled;
			if (m_enabled) OnEnable();
			else OnDisable();
		}

	private:
		void Invoke(MonoMethod* method)
		{
			if (!method || !object) return;
			MonoObject* exc = nullptr;
			mono_runtime_invoke(method, object, nullptr, &exc);
			if (exc) ReportManagedException(exc);
		}

		void InvokeArgs(MonoMethod* method, void** args)
		{
			if (!method || !object) return;
			MonoObject* exc = nullptr;
			mono_runtime_invoke(method, object, args, &exc);
			if (exc) ReportManagedException(exc);
		}

		void InvokeWithCollision(MonoMethod* method, EntityID other, bool isTrigger)
		{
			if (!method || !object) return;

			auto scriptSystem = ECS::GetInstance().GetSystem<ScriptSystem>();
			auto* engine = scriptSystem->m_ScriptEngine.get();
			MonoAssembly* apiAsm = engine->GetAPIAsm();
			if (!apiAsm) { EE_CORE_ERROR("ScriptInstance: API assembly not available"); return; }
			
			// Crafting the collision object (collider, gameobject, rigidbody, transform)

			MonoImage* apiImage = mono_assembly_get_image(apiAsm);
			MonoClass* colClass = mono_class_from_name(apiImage, "ErmineEngine", "Collision");
			if (!colClass) { EE_CORE_ERROR("ScriptInstance: ErmineEngine.Collision not found"); return; }
			MonoClass* colliderClass = mono_class_from_name(apiImage, "ErmineEngine", "Collider");
			if (!colliderClass) { EE_CORE_ERROR("ScriptInstance: ErmineEngine.Collider not found"); return; }
			MonoClass* goClass = mono_class_from_name(apiImage, "ErmineEngine", "GameObject");
			if (!goClass) { EE_CORE_ERROR("ScriptInstance: ErmineEngine.GameObject not found"); return; }
			MonoClass* rbClass = mono_class_from_name(apiImage, "ErmineEngine", "Rigidbody");
			if (!rbClass) { EE_CORE_ERROR("ScriptInstance: ErmineEngine.Rigidbody not found"); return; }
			MonoClass* tfClass = mono_class_from_name(apiImage, "ErmineEngine", "Transform");
			if (!tfClass) { EE_CORE_ERROR("ScriptInstance: ErmineEngine.Transform not found"); return; }

			// Setting up fields
			MonoObject* colObj = mono_object_new(engine->GetGameDomain(), colClass);
			if (!colObj) { EE_CORE_ERROR("ScriptInstance: Failed to allocate Collision"); return; }
			mono_runtime_object_init(colObj);

			MonoObject* colliderObj = mono_object_new(engine->GetGameDomain(), colliderClass);
			if (!colliderClass) { EE_CORE_ERROR("ScriptInstance: Failed to allocate Collider"); return; }
			mono_runtime_object_init(colliderObj);
			MonoObject* goObj = mono_object_new(engine->GetGameDomain(), goClass);
			if (!goObj) { EE_CORE_ERROR("ScriptInstance: Failed to allocate GameObject"); return; }
			//mono_runtime_object_init(goObj);
			MonoObject* rbObj = mono_object_new(engine->GetGameDomain(), rbClass);
			if (!rbObj) { EE_CORE_ERROR("ScriptInstance: Failed to allocate Rigidbody"); return; }
			mono_runtime_object_init(rbObj);
			MonoObject* tfObj = mono_object_new(engine->GetGameDomain(), tfClass);
			if (!tfObj) { EE_CORE_ERROR("ScriptInstance: Failed to allocate Transform"); return; }
			mono_runtime_object_init(tfObj);
			// Collider
			if (MonoClassField* field = mono_class_get_field_from_name(colliderClass, "EntityID")) {
				EntityID id = other;
				mono_field_set_value(colliderObj, field, &id);
			}
			// GameObject
			if (MonoClassField* field = mono_class_get_field_from_name(goClass, "EntityID"))
			{
				EntityID id = other;
				mono_field_set_value(goObj, field, &id);
			}
			// Rigidbody
			if (MonoClassField* field = mono_class_get_field_from_name(rbClass, "EntityID"))
			{
				EntityID id = other;
				mono_field_set_value(rbObj, field, &id);
			}
			// Transform
			if (MonoClassField* field = mono_class_get_field_from_name(tfClass, "EntityID"))
			{
				EntityID id = other;
				mono_field_set_value(tfObj, field, &id);
			}

			// Set fields of Collision
			if (MonoClassField* f = mono_class_get_field_from_name(colliderClass, "isTrigger")) mono_field_set_value(colliderObj, f, &isTrigger);
			if (MonoClassField* f = mono_class_get_field_from_name(colClass, "Collider"))   mono_field_set_value(colObj, f, &colliderObj);
			if (MonoClassField* f = mono_class_get_field_from_name(colClass, "GameObject")) mono_field_set_value(colObj, f, &goObj);
			if (MonoClassField* f = mono_class_get_field_from_name(colClass, "Rigidbody"))  mono_field_set_value(colObj, f, &rbObj);
			if (MonoClassField* f = mono_class_get_field_from_name(colClass, "Transform"))   mono_field_set_value(colObj, f, &tfObj);

			void* args[1]{ colObj };
			InvokeArgs(method , args);
		}

		void ReportManagedException(MonoObject* exc)
		{
			MonoString* s = mono_object_to_string(exc, nullptr);
			char* utf8 = mono_string_to_utf8(s);
			EE_CORE_ERROR("[C# Exception] {}", utf8 ? utf8 : "<null>");
			if (utf8) mono_free(utf8);
		}

		void InjectEntityIfAvailable() const
		{
			if (!object || !klass) return;

			if (MonoClassField* field = mono_class_get_field_from_name(klass->Class, "EntityID")) {
				EntityID id = entityID;
				mono_field_set_value(object, field, &id);
			}
		}
	};
}
