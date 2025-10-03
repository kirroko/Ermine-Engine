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
#include "EditorGUI.h"
#include "Logger.h"

Ermine::scripting::ScriptSystem::ScriptSystem()
{
	EE_CORE_TRACE("Script System initialing...");
	m_ScriptEngine = std::make_unique<ScriptEngine>();
	m_ScriptEngine->InitMono("../Ermine-ScriptAssembly/Ermine-ScriptAssembly.dll"); // TODO: Move dll into editor's build directory
	m_ScriptEngine->LoadGameAssembly("../Ermine-ScriptSandbox/Ermine-ScriptSandbox.dll"); // TODO: Move dll into editor's build directory

	// Configure MSBuild + source watcher (adjust paths as necessary)
#ifdef _DEBUG // We only want to do this in debug cause debug in editor mode
	// Start DLL watcher
	m_ScriptEngine->StartWatchingGameAssembly();
	m_ScriptEngine->ConfigureBuild(
		"../../../../Ermine-ScriptSandbox/Ermine-ScriptSandbox.csproj",
		"../../../../Ermine-ScriptSandbox",
		"../Ermine-ScriptSandbox/Ermine-ScriptSandbox.dll",
		"Debug",
		"x64", "MSBuild.exe");
	m_ScriptEngine->StartWatchingScriptSources();
#endif
}

void Ermine::scripting::ScriptSystem::Update() const
{
	static bool s_wasStopped = false;

	if (editor::EditorGUI::s_state == editor::EditorGUI::SimState::stopped)
	{
		if (s_wasStopped)
			return;

		for (auto& entity : m_Entities)
		{
			auto& sc = ECS::GetInstance().GetComponent<Script>(entity);
			sc.m_started = false;
		}
		s_wasStopped = true;
		return;
	}

	s_wasStopped = false;

#ifdef _DEBUG
	m_ScriptEngine->ProcessHotReload(
		[this]() { this->PrepareForHotReload(); },
		[this](bool ok) { this->FinishHotReload(ok); }
	);
#endif

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
	static bool s_wasStopped = false;

	if (editor::EditorGUI::s_state == editor::EditorGUI::SimState::stopped)
	{
		if (s_wasStopped)
			return;

		for (auto& entity : m_Entities)
		{
			auto& sc = ECS::GetInstance().GetComponent<Script>(entity);
			sc.m_started = false;
		}
		s_wasStopped = true;
		return;
	}

	s_wasStopped = false;

#ifdef _DEBUG
	m_ScriptEngine->ProcessHotReload(
		[this]() { this->PrepareForHotReload(); },
		[this](bool ok) { this->FinishHotReload(ok); }
	);
#endif

	for (auto& entity : m_Entities)
	{
		auto& sc = ECS::GetInstance().GetComponent<Script>(entity);

		if (!sc.m_enabled) continue;

		if (!sc.m_started) { sc.m_instance->Start(); sc.m_started = true; }

		sc.m_instance->FixedUpdate();
	}
}

void Ermine::scripting::ScriptSystem::PrepareForHotReload() const
{
	m_RestoreList.clear();
	auto& ecs = ECS::GetInstance();

	for (auto& entity : m_Entities)
	{
		if (!ecs.IsEntityValid(entity) || !ecs.HasComponent<Script>(entity))
			continue;

		auto& sc = ecs.GetComponent<Script>(entity);
		m_RestoreList.emplace_back(entity, sc.m_className);

		// Dispose existing managed instance
		sc.m_instance.reset();
		sc.m_started = false;
	}
}

void Ermine::scripting::ScriptSystem::FinishHotReload(bool success) const
{
	if (!success)
	{
		EE_CORE_ERROR("ScriptSystem: HotReload failed; skipping recreation.");
		m_RestoreList.clear();
		return;
	}

	auto& ecs = ECS::GetInstance();

	for (auto& [entity, className] : m_RestoreList)
	{
		if (!ecs.IsEntityValid(entity) || !ecs.HasComponent<Script>(entity))
			continue;

		auto& sc = ecs.GetComponent<Script>(entity);

		// REcreate instance with same class + entity
		sc.m_className = className;
		auto scriptClass = std::make_unique<ScriptClass>(ScriptClass("", className));
		sc.m_instance = std::make_unique<ScriptInstance>(std::move(scriptClass), entity);
		sc.m_started = false;
	}

	m_RestoreList.clear();
	EE_CORE_INFO("ScriptSystem: HotReload recreation complete.");
}
