workspace "Ermine"
    architecture "x64"
    configurations { "Editor-Debug", "Editor-Release", "Game-Debug", "Game-Release" }
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
IncludeDir["xproperty"] = "ThirdParty/xproperty/source"

-- Libraries
LibraryDir = {}
LibraryDir["Fmod"] = "ThirdParty/Fmod/lib"
LibraryDir["Mono"] = "ThirdParty/Mono/lib"
LibraryDir["assimp"] = "ThirdParty/assimp/lib"

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
        "%{IncludeDir.xproperty}"
    }

    libdirs
    {
        "%{LibraryDir.Fmod}",
        "%{LibraryDir.Mono}",
        "%{LibraryDir.assimp}"
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
        "assimp-vc143-mt.lib"
    }

    postbuildcommands
    {
        -- Compute base output dir once per config
        ("set OUTDIR=$(SolutionDir)Build\\bin\\" .. outputdir),

        -- Copy engine DLL to Editor and Game
        "{COPY} \"$(TargetPath)\" \"%OUTDIR%\\Ermine-Editor\"",
        "{COPY} \"$(TargetPath)\" \"%OUTDIR%\\Ermine-Game\"",

        -- Runtime DLLs (Editor)
        "{COPY} \"$(SolutionDir)ThirdParty\\Fmod\\lib\\fmod.dll\" \"%OUTDIR%\\Ermine-Editor\"",
        "{COPY} \"$(SolutionDir)ThirdParty\\Fmod\\lib\\fmodL.dll\" \"%OUTDIR%\\Ermine-Editor\"",
        "{COPY} \"$(SolutionDir)ThirdParty\\Fmod\\lib\\fmodstudio.dll\" \"%OUTDIR%\\Ermine-Editor\"",
        "{COPY} \"$(SolutionDir)ThirdParty\\Fmod\\lib\\fmodstudioL.dll\" \"%OUTDIR%\\Ermine-Editor\"",
        "{COPY} \"$(SolutionDir)ThirdParty\\Mono\\lib\\mono-2.0-sgen.dll\" \"%OUTDIR%\\Ermine-Editor\"",
        "{COPY} \"$(SolutionDir)ThirdParty\\assimp\\bin\\assimp-vc143-mt.dll\" \"%OUTDIR%\\Ermine-Editor\"",

        -- Runtime DLLs (Game)
        "{COPY} \"$(SolutionDir)ThirdParty\\Fmod\\lib\\fmod.dll\" \"%OUTDIR%\\Ermine-Game\"",
        "{COPY} \"$(SolutionDir)ThirdParty\\Fmod\\lib\\fmodL.dll\" \"%OUTDIR%\\Ermine-Game\"",
        "{COPY} \"$(SolutionDir)ThirdParty\\Fmod\\lib\\fmodstudio.dll\" \"%OUTDIR%\\Ermine-Game\"",
        "{COPY} \"$(SolutionDir)ThirdParty\\Fmod\\lib\\fmodstudioL.dll\" \"%OUTDIR%\\Ermine-Game\"",
        "{COPY} \"$(SolutionDir)ThirdParty\\Mono\\lib\\mono-2.0-sgen.dll\" \"%OUTDIR%\\Ermine-Game\"",
        "{COPY} \"$(SolutionDir)ThirdParty\\assimp\\bin\\assimp-vc143-mt.dll\" \"%OUTDIR%\\Ermine-Game\"",

        -- Mono redist
        "{COPY} \"$(SolutionDir)ThirdParty\\Mono\\lib\" \"%OUTDIR%\\Ermine-Editor\\mono\\lib\"",
        "{COPY} \"$(SolutionDir)ThirdParty\\Mono\\etc\" \"%OUTDIR%\\Ermine-Editor\\mono\\etc\"",
        "{COPY} \"$(SolutionDir)ThirdParty\\Mono\\lib\" \"%OUTDIR%\\Ermine-Game\\mono\\lib\"",
        "{COPY} \"$(SolutionDir)ThirdParty\\Mono\\etc\" \"%OUTDIR%\\Ermine-Game\\mono\\etc\"",

        -- Resources (shared)
        "{COPY} \"$(SolutionDir)Resources\" \"%OUTDIR%\\Resources\""
    }

    filter "configurations:Editor-Debug or configurations:Game-Debug"
        links { "fmodL_vc", "fmodstudioL_vc" }

    filter "configurations:Editor-Release or configurations:Game-Release"
        links { "fmod_vc", "fmodstudio_vc" }

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

    -- Editor vs Game feature flags for the engine build
    filter "configurations:*Editor*"
        defines { "EE_EDITOR" }

    filter "configurations:*Game*"
        defines { "EE_GAME" }

    filter "configurations:Editor-Debug or configurations:Game-Debug"
        defines "EE_DEBUG"
        runtime "Debug"
        symbols "on"
        linkoptions { "/NODEFAULTLIB:LIBCMTD", "/NODEFAULTLIB:LIBCMT", "/NODEFAULTLIB:MSVCRT" }
        defines { "VERBOSE_LOGGING=1" }

    filter "configurations:Editor-Release or configurations:Game-Release"
        defines "EE_RELEASE"
        runtime "Release"
        optimize "on"
        linkoptions { "/NODEFAULTLIB:LIBCMT", "/NODEFAULTLIB:LIBCMTD", "/NODEFAULTLIB:MSVCRTD" }
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
        "%{IncludeDir.rapidjson}",
        "%{IncludeDir.xproperty}"
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

    filter "configurations:*Editor*"
        defines { "EE_EDITOR" }

    filter "configurations:*Game*"
        defines { "EE_GAME" } -- if selected, nothing SHOULD happen

    filter "configurations:Editor-Debug"
        defines "EE_DEBUG"
        runtime "Debug"
        symbols "on"
        linkoptions { "/NODEFAULTLIB:LIBCMTD" }

    filter "configurations:Editor-Release"
        defines "EE_RELEASE"
        runtime "Release"
        optimize "on"
        linkoptions { "/NODEFAULTLIB:LIBCMT" }

-- Game Project
project "Ermine-Game"
    location "Ermine-Game"
    kind "WindowedApp"
    language "C++"
    cppdialect "C++20"
    staticruntime "off"

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
        -- "%{IncludeDir.ImGui}", -- May not be needed in the game
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
        buildoptions { "/wd4251", "/wd4005" }
        defines { "EE_PLATFORM_WINDOWS" }

    filter "configurations:*Game*"
        defines { "EE_GAME" }

    filter "configurations:Game-Debug"
        defines "EE_DEBUG"
        runtime "Debug"
        symbols "on"
        linkoptions { "/NODEFAULTLIB:LIBCMTD" }

    filter "configurations:Game-Release"
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

    files { "%{prj.name}/**.cs" }

    filter "system:windows"
        systemversion "latest"
    filter "configurations:Editor-Debug or configurations:Game-Debug"
        defines { "DEBUG" }
        symbols "on"
    filter "configurations:Editor-Release or configurations:Game-Release"
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

    files { "%{prj.name}/**.cs" }
    includedirs { "Ermine-ScriptAssembly" }
    links { "Ermine-ScriptAssembly" }

    filter "system:windows"
        systemversion "latest"
    filter "configurations:Editor-Debug or configurations:Game-Debug"
        defines { "DEBUG" }
        symbols "on"
    filter "configurations:Editor-Release or configurations:Game-Release"
        defines { "NDEBUG" }
        optimize "on"