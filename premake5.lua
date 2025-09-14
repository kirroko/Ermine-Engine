workspace "Ermine"
    architecture "x64"
    configurations { "Debug", "Release"}
    startproject "Ermine-Editor"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
fmod_dll = "../ThirdParty/Fmod/lib/fmod.dll"
mono_dll = "../ThirdParty/Mono/lib/mono-2.0-sgen.dll"
mono_assembly = "../ThirdParty/Mono/lib/"
mono_config = "../ThirdParty/Mono/etc"

-- Include directories relative to root folder (solution directory)
IncludeDir = {}
IncludeDir["GLFW"] = "ThirdParty/GLFW/include"
IncludeDir["Glad"] = "ThirdParty/Glad/include"
IncludeDir["ImGui"] = "ThirdParty/imgui"
IncludeDir["glm"] = "ThirdParty/glm"
IncludeDir["spdlog"] = "ThirdParty/spdlog/include"
IncludeDir["stb"] = "ThirdParty/stb"
IncludeDir["Mono"] = "ThirdParty/Mono/include"

-- Libraries
LibraryDir = {}
LibraryDir["Fmod"] = "ThirdParty/Fmod/lib"
LibraryDir["Mono"] = "ThirdParty/Mono/lib"

-- External libraries
group "Dependencies"
    include "ThirdParty/GLFW"
    include "ThirdParty/Glad"
    include "ThirdParty/imgui"
    include "ThirdParty/Fmod"
    include "ThirdParty/Mono"
group ""

-- Engine Project
project "Ermine-Engine"
    location "Ermine-Engine"
    kind "SharedLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "off" -- Use dynamic runtime

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
        "%{IncludeDir.Mono}"
    }

    libdirs
    {
        "%{LibraryDir.Fmod}",
        "%{LibraryDir.Mono}"
    }

    links
    {
        "GLFW",
        "Glad",
        "ImGui",
        "fmod_vc",
        "opengl32.lib",
		"mono-2.0-sgen.lib"
    }

    postbuildcommands
    {
        ("{COPY} %{cfg.buildtarget.relpath} ../Build/bin/" .. outputdir .. "/Ermine-Editor"),
        ("{COPY} " .. fmod_dll .. " ../Build/bin/" .. outputdir .. "/Ermine-Editor"),
        ("{COPY} " .. mono_dll .. " ../Build/bin/" .. outputdir .. "/Ermine-Editor"),
        ("{COPYDIR} " .. mono_assembly .. " ../Build/bin/" .. outputdir .. "/Ermine-Editor/mono/lib"),
        ("{COPYDIR} " .. mono_config .. " ../Build/bin/" .. outputdir .. "/Ermine-Editor/mono/etc"),
        ("{COPYDIR} ../Resources ../Build/bin/" .. outputdir .. "/Resources")
    }

    filter "system:windows"
        systemversion "latest"

        warnings "Extra"

        buildoptions { "/wd4251", "/wd4005", "/wd4267" }

        defines
        {
            "EE_PLATFORM_WINDOWS",
            "EE_BUILD_DLL",
            "GLFW_INCLUDE_NONE"
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
        "%{IncludeDir.Mono}"
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

    targetdir ("Build/bin/" .. outputdir .. "/%{prj.name}")
    objdir ("Build/obj/" .. outputdir .. "/%{prj.name}")

    files
    {
        "%{prj.name}/**.cs",
        "%{prj.name}/**.csproj"
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

    targetdir ("Build/bin/" .. outputdir .. "/%{prj.name}")
    objdir ("Build/obj/" .. outputdir .. "/%{prj.name}")

    files
    {
        "%{prj.name}/**.cs",
        "%{prj.name}/**.csproj"
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