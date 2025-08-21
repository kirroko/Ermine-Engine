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

#include "FrameController.h"
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

	m_gameDomain = mono_domain_create_appdomain("ErmineGame", nullptr);
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
	float icall_time_get_deltatime() { EE_CORE_INFO("Delta Time asked!"); return Ermine::FrameController::GetDeltaTime(); }
	float icall_time_get_fixeddeltatime() { return Ermine::FrameController::GetFixedDeltaTime(); }

	const char* ToTempUTF8(MonoString* str, std::string& out)
	{
		if (!str)
		{
			out.clear();
			return out.c_str();
		}
		char* raw = mono_string_to_utf8(str);
		out = raw ? raw : "";
		if (raw)
			mono_free(raw);
		return out.c_str();
	}

	void icall_debug_log_info(MonoString* message)
	{
		std::string temp;
		EE_CORE_INFO("{}", ToTempUTF8(message,temp));
	}

	void icall_debug_log_warning(MonoString* message)
	{
		std::string temp;
		EE_CORE_WARN("{}", ToTempUTF8(message,temp));
	}

	void icall_debug_log_error(MonoString* message)
	{
		std::string temp;
		EE_CORE_ERROR("{}", ToTempUTF8(message,temp));
	}
}

void Ermine::scripting::ScriptEngine::RegisterInternalCalls()
{
	mono_add_internal_call("ErmineEngine.Time::get_deltaTime",	(const void*)icall_time_get_deltatime);
	mono_add_internal_call("ErmineEngine.Time::get_fixedDeltaTime", (const void*)icall_time_get_fixeddeltatime);

	mono_add_internal_call("ErmineEngine.Debug::LogInternal",			(const void*)icall_debug_log_info);
	mono_add_internal_call("ErmineEngine.Debug::LogWarningInternal",	(const void*)icall_debug_log_warning);
	mono_add_internal_call("ErmineEngine.Debug::LogErrorInternal",		(const void*)icall_debug_log_error);
}


