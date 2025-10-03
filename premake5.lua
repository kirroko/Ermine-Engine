workspace "Ermine"
    architecture "x64"
    configurations { "Debug", "Release"}
    startproject "Ermine-Editor"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
fmod_dll = "../ThirdParty/Fmod/lib/fmod.dll"
fmodL_dll = "../ThirdParty/Fmod/lib/fmodL.dll"
fmodstudio_dll = "../ThirdParty/Fmod/lib/fmodstudio.dll"
fmodstudioL_dll = "../ThirdParty/Fmod/lib/fmodstudioL.dll"
mono_dll = "../ThirdParty/Mono/lib/mono-2.0-sgen.dll"
mono_assembly = "../ThirdParty/Mono/lib/"
mono_config = "../ThirdParty/Mono/etc"
assimp_dll = "../ThirdParty/assimp/bin/assimp-vc143-mt.dll"

-- Include directories relative to root folder (solution directory)
IncludeDir = {}
IncludeDir["GLFW"] = "ThirdParty/GLFW/include"
IncludeDir["Glad"] = "ThirdParty/Glad/include"
IncludeDir["ImGui"] = "ThirdParty/imgui"
IncludeDir["glm"] = "ThirdParty/glm"
IncludeDir["spdlog"] = "ThirdParty/spdlog/include"
IncludeDir["stb"] = "ThirdParty/stb"
IncludeDir["Mono"] = "ThirdParty/Mono/include"
IncludeDir["rapidjson"] = "ThirdParty/rapidjson"
IncludeDir["Fmod"] = "ThirdParty/Fmod/inc"
IncludeDir["Jolt"] = "ThirdParty"
IncludeDir["assimp"] = "ThirdParty/assimp/include"
IncludeDir["DirectXTex"] = "ThirdParty/DirectXTex/inc"

-- Libraries
LibraryDir = {}
LibraryDir["Fmod"] = "ThirdParty/Fmod/lib"
LibraryDir["Mono"] = "ThirdParty/Mono/lib"
LibraryDir["assimp"] = "ThirdParty/assimp/lib"
LibraryDir["DirectXTex"] = "ThirdParty/DirectXTex/lib"

-- External libraries
group "Dependencies"
    include "ThirdParty/GLFW"
    include "ThirdParty/Glad"
    include "ThirdParty/imgui"
    include "ThirdParty/Fmod"
    include "ThirdParty/Mono"
    include "ThirdParty/Jolt"
group ""

-- Engine Project
project "Ermine-Engine"
    location "Ermine-Engine"
    kind "SharedLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "off" -- Use dynamic runtime

    buildoptions { "/MP" } -- Enable multi-processor compilation

    targetdir ("Build/bin/" .. outputdir .. "/%{prj.name}")
    objdir ("Build/obj/" .. outputdir .. "/%{prj.name}")

    pchheader "PreCompile.h"
    pchsource "Ermine-Engine/src/PreCompile.cpp"

    files
    {
        "%{prj.name}/include/**.h",
        "%{prj.name}/include/**.tpp",
        "%{prj.name}/src/**.tpp",
        "%{prj.name}/src/**.cpp"
    }

    includedirs
    {
        "%{prj.name}/include",
        "%{IncludeDir.GLFW}",
        "%{IncludeDir.Glad}",
        "%{IncludeDir.ImGui}",
        "%{IncludeDir.glm}",
        "%{IncludeDir.spdlog}",
        "%{IncludeDir.stb}",
        "%{IncludeDir.Fmod}",
        "%{IncludeDir.Mono}",
        "%{IncludeDir.Jolt}",
        "%{IncludeDir.rapidjson}",
        "%{IncludeDir.assimp}",
        "%{IncludeDir.DirectXTex}"
    }

    libdirs
    {
        "%{LibraryDir.Fmod}",
        "%{LibraryDir.Mono}",
        "%{LibraryDir.assimp}",
        "%{LibraryDir.DirectXTex}"
    }

    links
    {
        "GLFW",
        "Glad",
        "ImGui",
        "fmod_vc",
        "fmodL_vc",
        "fmodstudio_vc",
        "fmodstudioL_vc",
        "opengl32.lib",
		"mono-2.0-sgen.lib",
        "Jolt",
        "assimp-vc143-mt.lib",
        "DirectXTex.lib"
    }

    postbuildcommands
    {
        ("{COPY} %{cfg.buildtarget.relpath} ../Build/bin/" .. outputdir .. "/Ermine-Editor"),
        ("{COPY} " .. fmod_dll .. " ../Build/bin/" .. outputdir .. "/Ermine-Editor"),
        ("{COPY} " .. fmodL_dll .. " ../Build/bin/" .. outputdir .. "/Ermine-Editor"),
        ("{COPY} " .. fmodstudio_dll .. " ../Build/bin/" .. outputdir .. "/Ermine-Editor"),
        ("{COPY} " .. fmodstudioL_dll .. " ../Build/bin/" .. outputdir .. "/Ermine-Editor"),
        ("{COPY} " .. mono_dll .. " ../Build/bin/" .. outputdir .. "/Ermine-Editor"),
        ("{COPY} " .. assimp_dll .. " ../Build/bin/" .. outputdir .. "/Ermine-Editor"),
        ("{COPYDIR} " .. mono_assembly .. " ../Build/bin/" .. outputdir .. "/Ermine-Editor/mono/lib"),
        ("{COPYDIR} " .. mono_config .. " ../Build/bin/" .. outputdir .. "/Ermine-Editor/mono/etc"),
        ("{COPYDIR} ../Resources ../Build/bin/" .. outputdir .. "/Resources"),
        ("{COPYDIR} ../Ermine-ResourcePipeline/Ermine-Game.lion_rcdbase ../Build/bin/" .. outputdir .. "/Ermine-Game.lion_rcdbase"),
        ("{COPYDIR} ../Ermine-ResourcePipeline/Ermine-Game.lion_project ../Build/bin/" .. outputdir .. "/Ermine-Game.lion_project"),
        ("{COPY} %{cfg.buildtarget.relpath} ../Build/bin/" .. outputdir .. "/Ermine-Editor/Jolt")
    }

    filter "system:windows"
        systemversion "latest"

        warnings "Extra"

        buildoptions {  "/wd4251", -- Level 2 dll-interface to be used by clients of class 'class'; see C4251
                        "/wd4005", -- Level 1 macro redefinition
                        "/wd4267", -- Level 3 conversion from 'size_t' to 'type', possible loss of data
                        "/wd5054"  -- Level 4 operator '|': deprecated between enumerations of different types (Due to Rapidjson library at document.h)
        }

        defines
        {
            "EE_PLATFORM_WINDOWS",
            "EE_BUILD_DLL",
            "GLFW_INCLUDE_NONE",
            "IMGUI_DEFINE_MATH_OPERATORS",
            "GLM_ENABLE_EXPERIMENTAL",
            "_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING" -- To slience the warnings from Rapidjson
        }

    filter "configurations:Debug"
        defines "EE_DEBUG"
        runtime "Debug"
        symbols "on"
        linkoptions { "/NODEFAULTLIB:LIBCMTD" }

        defines { "VERBOSE_LOGGING=1" }

    filter "configurations:Release"
        defines "EE_RELEASE"
        runtime "Release"
        optimize "on"
        linkoptions { "/NODEFAULTLIB:LIBCMT" }
        
        defines { "VERBOSE_LOGGING=0" }

-- Editor Project
project "Ermine-Editor"
    location "Ermine-Editor"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    staticruntime "off" -- Use dynamic runtime

    targetdir ("Build/bin/" .. outputdir .. "/%{prj.name}")
    objdir ("Build/obj/" .. outputdir .. "/%{prj.name}")
    debugdir ("Build/bin/" .. outputdir .. "/%{prj.name}")

    files
    {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp",
    }

    includedirs
    {
        "%{prj.name}/include",
        "Ermine-Engine/include",
        "%{IncludeDir.GLFW}",
        "%{IncludeDir.Glad}",
        "%{IncludeDir.ImGui}",
        "%{IncludeDir.glm}",
        "%{IncludeDir.spdlog}",
        "%{IncludeDir.stb}",
        "%{IncludeDir.Fmod}",
        "%{IncludeDir.Mono}",
        "%{IncludeDir.rapidjson}"
    }

    links
    {
        "Ermine-Engine"
    }

    filter "system:windows"
        systemversion "latest"

        buildoptions { "/wd4251", "/wd4005"}

        defines
        {
            "EE_PLATFORM_WINDOWS"
        }

    filter "configurations:Debug"
        defines "EE_DEBUG"
        runtime "Debug"
        symbols "on"
        linkoptions { "/NODEFAULTLIB:LIBCMTD" }

    filter "configurations:Release"
        defines "EE_RELEASE"
        runtime "Release"
        optimize "on"
        linkoptions { "/NODEFAULTLIB:LIBCMT" }

-- Script Assembly Project
project "Ermine-ScriptAssembly"
    location "Ermine-ScriptAssembly"
    kind "SharedLib"
    language "C#"
    dotnetframework "4.7.2"
    namespace "Ermine.ScriptAssembly"

    targetdir ("Build/bin/" .. outputdir .. "/%{prj.name}")
    objdir ("Build/obj/" .. outputdir .. "/%{prj.name}")

    files
    {
        "%{prj.name}/**.cs"
    }

    filter "system:windows"
        systemversion "latest"
    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "on"
    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "on"

-- Script Sandbox project
project "Ermine-ScriptSandbox"
    location "Ermine-ScriptSandbox"
    kind "SharedLib"
    language "C#"
    dotnetframework "4.7.2"
    namespace "Ermine.ScriptSandbox"

    targetdir ("Build/bin/" .. outputdir .. "/%{prj.name}")
    objdir ("Build/obj/" .. outputdir .. "/%{prj.name}")

    files
    {
        "%{prj.name}/**.cs"
    }
    includedirs
    {
        "Ermine-ScriptAssembly"
    }
    links
    {
        "Ermine-ScriptAssembly"
    }
    filter "system:windows"
        systemversion "latest"
    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "on"
    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "on"