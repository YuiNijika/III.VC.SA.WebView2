-- Premake Project Generator for III.VC.SA.WebView2

local XBASE_LIB_DIR = "lib"

workspace "WebView2"
    configurations { "Debug", "Release" }
    architecture "x86"
    platforms "Win32"
    language "C++"
    cppdialect "C++20"
    characterset "MBCS"
    staticruntime "On"
    location "build"
    targetdir "build/bin"

    toolset "msc"
    buildoptions { "/utf-8", "/FS" }

    defines {
        "IS_PLATFORM_WIN",
        "_CRT_SECURE_NO_WARNINGS",
        "_CRT_NON_CONFORMING_SWPRINTFS"
    }

function configureBuildMode()
    filter "configurations:Debug"
        symbols "On"
        defines { "DEBUG" }

    filter "configurations:Release"
        optimize "On"
        defines { "NDEBUG" }

    filter {}
end

-- 载荷命名与宿主 ASI 名解耦，loader 导出基名 WebView2，
-- 主 ASI 仍叫 III.VC.SA.WebView2.asi，载荷是 WebView2\WebView2SA.dll
local PAYLOAD_BASE = "WebView2"

function createPayloadProject(projectID)
    local upperID = string.upper(projectID)
    local gameDefine = upperID == "III" and "GTA3" or "GTA" .. upperID
    local pluginName = upperID == "III" and "PluginIII" or "Plugin" .. upperID

    project ("WebView2Payload" .. upperID)
        kind "SharedLib"
        targetname (PAYLOAD_BASE .. upperID)
        targetextension ".dll"
        targetdir ("build/bin/" .. PAYLOAD_BASE)

        includedirs {
            "include",
            "src"
        }

        files {
            "src/**.h",
            "src/**.hpp",
            "src/**.c",
            "src/**.cpp",
            "src/**.rc",
            "include/XBase/**.h"
        }

        if upperID ~= "SA" then
            removefiles { "src/**_sa.c", "src/**_sa.hpp", "src/**_sa.cpp" }
        end
        if upperID ~= "VC" then
            removefiles { "src/**_vc.c", "src/**_vc.hpp", "src/**_vc.cpp" }
        end
        if upperID ~= "III" then
            removefiles { "src/**_iii.c", "src/**_iii.hpp", "src/**_iii.cpp" }
        end

        defines { gameDefine }
        libdirs { XBASE_LIB_DIR }

        links { "XBase" .. upperID, pluginName, "XBasePayloadEntry" }
        linkoptions { "/WHOLEARCHIVE:XBasePayloadEntry.lib" }

        configureBuildMode()
end

function createLoaderProject()
    project "WebView2"
        kind "SharedLib"
        targetname (PAYLOAD_BASE)
        targetextension ".asi"

        files {
            "loader/**.h",
            "loader/**.cpp"
        }

        libdirs { XBASE_LIB_DIR }
        links { "XBaseBootstrap" }
        linkoptions { "/WHOLEARCHIVE:XBaseBootstrap.lib" }

        configureBuildMode()
end

createPayloadProject("sa")
createPayloadProject("vc")
createPayloadProject("iii")
createLoaderProject()
