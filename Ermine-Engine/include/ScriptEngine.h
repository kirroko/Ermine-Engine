/* Start Header ************************************************************************/
/*!
\file       ScriptEngine.h
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
#pragma once
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>

namespace Ermine::scripting
{
	class ScriptEngine
	{
		MonoDomain* m_coreDomain = nullptr;
		MonoDomain* m_gameDomain = nullptr;
		MonoAssembly* m_apiAsm = nullptr;
		MonoImage* m_apiImage = nullptr;
		MonoAssembly* m_gameAsm = nullptr;
		MonoImage* m_gameImage = nullptr;
	public:
		/**
		 * @brief Initialize the Mono runtime and create the core and game domains.
		 * This function should be called before any other functions in this class.
		 * @param assembly_path The path to the C# assembly to load.
		 */
		void InitMono(const std::string& assembly_path);
		/**
		 * @brief Shutdown the Mono runtime and clean up resources.
		 */
		void Shutdown();
		/**
		 * @brief Print all types in the given assembly.
		 * @param assembly The MonoAssembly to print types from.
		 */
		void PrintAssemblyTypes(MonoAssembly* assembly);
		/**
		 * @brief Read the bytes from a file.
		 * @param filepath The path to the file to read.
		 * @param outSize The size of the file in bytes.
		 * @return A pointer to the bytes read from the file. The caller is responsible for freeing the memory.
		 */
		char* ReadBytes(const std::string& filepath, uint32_t* outSize);
		/**
		 * @brief Load a C# assembly from the given path.
		 * @param assemblyPath The path to the assembly to load.
		 * @return A pointer to the loaded MonoAssembly, or nullptr if the assembly could not be loaded.
		 */
		[[nodiscard]]
		MonoAssembly* LoadCSharpAssembly(const std::string& assemblyPath);
	};
}