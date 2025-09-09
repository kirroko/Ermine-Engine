/* Start Header ************************************************************************/
/*!
\file       ScriptSystem.cpp
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong\@digipen.edu
\date       09/08/2025
\brief      This files contains the declaration for the Script System.
			The Script System is responsible for managing the Mono script engine,
			loading scripts, and executing them. It provides an interface for the ECS
			to interact with the scripts, allowing for dynamic behavior in the game.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "PreCompile.h"
#include "ScriptSystem.h"

#include "Components.h"
#include "ECS.h"
#include "Logger.h"

Ermine::scripting::ScriptSystem::ScriptSystem()
{
	EE_CORE_TRACE("Script System initialing...");
	m_ScriptEngine = std::make_unique<ScriptEngine>();
	m_ScriptEngine->InitMono("../Ermine-ScriptAssembly/Ermine-ScriptAssembly.dll"); // TODO: Move dll into editor's build directory
	m_ScriptEngine->LoadGameAssembly("../Ermine-ScriptSandbox/Ermine-ScriptSandbox.dll"); // TODO: Move dll into editor's build directory
}

void Ermine::scripting::ScriptSystem::Update() const
{
	for (auto& entity : m_Entities)
	{
		auto& sc = ECS::GetInstance().GetComponent<Script>(entity);

		if (!sc.m_enabled) continue;

		if (!sc.m_started) { sc.m_instance->Start(); sc.m_started = true; }

		sc.m_instance->Update();
	}
}

void Ermine::scripting::ScriptSystem::FixedUpdate() const
{
	for (auto& entity : m_Entities)
	{
		auto& sc = ECS::GetInstance().GetComponent<Script>(entity);

		if (!sc.m_enabled) continue;

		if (!sc.m_started) { sc.m_instance->Start(); sc.m_started = true; }

		sc.m_instance->FixedUpdate();
	}
}
