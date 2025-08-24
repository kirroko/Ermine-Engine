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
	struct ScriptInstance
	{
		std::unique_ptr<ScriptClass> klass;
		MonoObject* object = nullptr;
		MonoGCHandle GCHandle = nullptr; // Mono GC handle for the script instance
		EntityID	entityID; // Entity ID this script instance is attached to

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
			Awake(); // called when an enabled script instance is being loaded.
			OnEnable(); // called when the object becomes enabled and active.
		}

		~ScriptInstance()
		{
			OnDisable();
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

	private:
		void Invoke(MonoMethod* method)
		{
			if (!method || !object) return;
			MonoObject* exc = nullptr;
			mono_runtime_invoke(method, object, nullptr, &exc);
			if (exc) ReportManagedException(exc);
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
