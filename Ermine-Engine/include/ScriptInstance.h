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
		Ermine::scripting::ScriptClass* klass;
		MonoObject* instance = nullptr;

		ScriptInstance(Ermine::scripting::ScriptClass* k) : klass(k)
		{
			EE_CORE_TRACE("Script Instance Ctor");
			instance = klass->Instantiate();
			// store the native pointer back into the C# object for callbacks if desired:
			// mono_field_set_value(instance, getInstancePtrField(), &instance);
		}

		void Start() const
		{
			if (klass->MethodStart)
				mono_runtime_invoke(klass->MethodStart, instance, nullptr, nullptr);
		}

		void Update() const
		{
			if (klass->MethodUpdate)
				mono_runtime_invoke(klass->MethodUpdate, instance, nullptr, nullptr);
		}
	};
}
