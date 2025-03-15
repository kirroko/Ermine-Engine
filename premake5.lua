workspace "Ermine"
    architecture "x64"
    configurations { "Debug", "Release"}
    startproject "Ermine-Editor"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Include directories relative to root folder (solution directory)
IncludeDir = {}
IncludeDir["GLFW"] = "ThirdParty/GLFW/include"
IncludeDir["Glad"] = "ThirdParty/Glad/include"
IncludeDir["ImGui"] = "ThirdParty/ImGui"
IncludeDir["glm"] = "ThirdParty/glm"
IncludeDir["spdlog"] = "ThirdParty/spdlog/include"
IncludeDir["stb"] = "ThirdParty/stb"

-- External libraries
group "Dependencies"
    include "ThirdParty/GLFW"
    include "ThirdParty/Glad"
    include "ThirdParty/ImGui"
group ""

-- Engine Project
project "Ermine-Engine"
    location "Ermine-Engine"
    kind "SharedLib"
    language "C++"
    cppdialect "C++17"
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
        "%{IncludeDir.stb}"
    }

    links
    {
        "GLFW",
        "Glad",
        "ImGui",
        "opengl32.lib"
    }

    postbuildcommands
    {
        ("{COPY} %{cfg.buildtarget.relpath} ../Build/bin/" .. outputdir .. "/Ermine-Editor"),
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

    filter "configurations:Release"
        defines "EE_RELEASE"
        runtime "Release"
        optimize "on"
        linkoptions { "/NODEFAULTLIB:LIBCMT" }

-- Editor Project
project "Ermine-Editor"
    location "Ermine-Editor"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    staticruntime "off" -- Use dynamic runtime

    targetdir ("Build/bin/" .. outputdir .. "/%{prj.name}")
    objdir ("Build/obj/" .. outputdir .. "/%{prj.name}")

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
        "%{IncludeDir.stb}"
    }

    links
    {
        "Ermine-Engine",
        "GLFW",
        "Glad",
        "ImGui",
        "opengl32.lib"
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