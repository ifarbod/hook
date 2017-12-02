project "Hook"
    language "C++"
    kind "StaticLib"
    
    vpaths
    {
        ["Headers/*"] = "**.hpp",
        ["Sources/*"] = "**.cpp",
        ["*"] = "premake5.lua"
    }

    files
    {
        "premake5.lua",
        "*.cpp",
        "*.hpp"
    }
