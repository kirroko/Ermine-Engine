project "Jolt"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"

    buildoptions { "/bigobj", "/MP" } -- Jolt requires this flag on MSVC

    targetdir ("../../Build/bin/" .. outputdir .. "/Ermine-Editor/Jolt")
    objdir ("../../Build/obj/" .. outputdir .. "/Ermine-Editor/Jolt")

	files
	{
	    "**.h",
		"**.cpp"
	}

    includedirs
    {
        "../"
    }


    filter "configurations:Debug"
        targetname "joltD"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        targetname "jolt"
        runtime "Release"
        optimize "on"

    filter "system:windows"
        systemversion "latest"