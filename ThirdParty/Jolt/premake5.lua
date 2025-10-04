project "Jolt"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"

    -- Match engine’s dynamic CRT to avoid LNK2038/LNK4098
    staticruntime "off"

    buildoptions { "/bigobj", "/MP" } -- Jolt requires this flag on MSVC (bigobj)

    -- Keep your existing output layout
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

    filter "configurations:*Debug"
        targetname "joltD"
        runtime "Debug"   -- /MDd
        symbols "on"

    filter "configurations:*Release"
        targetname "jolt"
        runtime "Release" -- /MD
        optimize "on"

    filter "system:windows"
        systemversion "latest"