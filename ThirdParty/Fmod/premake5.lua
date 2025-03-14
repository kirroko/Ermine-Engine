project "Fmod"
	kind "StaticLib"
	language "C++"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"inc/fmod.hpp"
	}


	filter "system:windows"
		systemversion "latest"
		staticruntime "On" --staticly linking the runtime libraries

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"