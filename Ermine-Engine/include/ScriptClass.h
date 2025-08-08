/* Start Header ************************************************************************/
/*!
\file       ScriptClass.h
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong\@digipen.edu
\date       07/08/2025
\brief      This file contains the metadata for one C# type

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/
#pragma once
#include "ECS.h"
#include "Logger.h"
#include "Logic.h"
#include <mono/jit/jit.h>

namespace Ermine::scripting
{
	struct ScriptClass
	{
		MonoClass* Class = nullptr; // Mono class pointer
		MonoMethod* Ctor = nullptr; // Constructor method pointer
		MonoMethod* MethodStart = nullptr; // Method to start the script
		MonoMethod* MethodUpdate = nullptr; // Method to update the script

		ScriptClass(const std::string& namespaceName, const std::string& className)
		{
			EE_CORE_TRACE("Script class ctor!");
			Class = mono_class_from_name(
				ECS::GetInstance().GetSystem<logic::ScriptSystem>()->m_ScriptSystem->GetGameImage(),
				namespaceName.c_str(),
				className.c_str()
			);
			Ctor = mono_class_get_method_from_name(Class, ".ctor", 0);
			MethodStart = mono_class_get_method_from_name(Class, "Start", 0);
			MethodUpdate = mono_class_get_method_from_name(Class, "Update", 0);
		}

		MonoObject* Instantiate() const
		{
			MonoObject* instance = mono_object_new(ECS::GetInstance().GetSystem<logic::ScriptSystem>()->m_ScriptSystem->GetDomain(), Class);
			mono_runtime_object_init(instance);
			return instance;
		}
	};
}

