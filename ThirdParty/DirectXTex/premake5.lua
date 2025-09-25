project "Fmod"
	kind "StaticLib"
	language "C++"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
	    "inc/**.h",
		"inc/**.hpp",
		"inc/**.inl"
	}

	links
	{
	    "DirectXTex.lib"
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