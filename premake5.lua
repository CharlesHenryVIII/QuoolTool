require "ecc/ecc"

workspace "ScadaBackup"
    configurations { "Debug", "Profile", "Release" }
    platforms { "x64", "x86" }
    staticruntime "On"
    runtime "Debug"

project "ScadaBackup"
    kind "ConsoleApp"--"WindowedApp"
    language "C++"
    cppdialect "C++20"
    targetdir "build/%{cfg.platform}/%{cfg.buildcfg}"
    objdir "build/obj/%{cfg.platform}/%{cfg.buildcfg}"

    targetname "ScadaBackup_%{cfg.system}_%{cfg.platform}_%{cfg.buildcfg}"
    objdir "build/obj/%{cfg.platform}/%{cfg.buildcfg}"

    editandcontinue "Off"
    usefullpaths ("On")
    --usestandardpreprocessor 'On'
    --characterset "ASCII"

    --Flags
    multiprocessorcompile "On"
    enablepch "Off"
    fatalwarnings { "All" }

    links {
        "OpenGL32",
    }

    libdirs {
        "contrib/SDL2/lib/%{cfg.platform}/",
        "contrib/ImGui",
        "contrib/tracy",
    }

    includedirs {
        "contrib",
        "contrib/ImGui",
        "contrib/SDL2/include",
        "contrib/tracy/public/tracy",
        "contrib/glfw/include",
    }
    fatalwarnings { "All" }
    defines {
        "_CRT_SECURE_NO_WARNINGS",
    }
    files {
        "src/**",
        "contrib/tracy/public/TracyClient.cpp",
        "contrib/ImGui/*.cpp",
        "contrib/ImGui/*.h",
        "contrib/ImGui/backends/imgui_impl_opengl3.*",
        "contrib/ImGui/backends/imgui_impl_glfw.*",
        "contrib/json.hpp",
        "resources/**",

        "contrib/glfw/src/internal.h",
        "contrib/glfw/src/platform.h",
        "contrib/glfw/src/mappings.h",
        "contrib/glfw/src/context.c",
        "contrib/glfw/src/init.c",
        "contrib/glfw/src/input.c",
        "contrib/glfw/src/monitor.c",
        "contrib/glfw/src/platform.c",
        "contrib/glfw/src/vulkan.c",
        "contrib/glfw/src/window.c",
        "contrib/glfw/src/egl_context.c",
        "contrib/glfw/src/osmesa_context.c",
        "contrib/glfw/src/null_platform.h",
        "contrib/glfw/src/null_joystick.h",
        "contrib/glfw/src/null_init.c",
        "contrib/glfw/src/null_monitor.c",
        "contrib/glfw/src/null_window.c",
        "contrib/glfw/src/null_joystick.c",
        "contrib/glfw/src/wgl_context.c",
        --"contrib/glfw/src/wlx_context.c",
    }

    filter "system:Windows"
        system "windows"
        defines {
        "WIN32",
        "_GLFW_WIN32",}

        files {
        "contrib/glfw/src/win32*",
        --"contrib/glfw/src/wlx_context.c",
    }

    filter "system:Unix"
        system "linux"
        defines {
        "LINUX",
        "_GLFW_WAYLAND",
        "_GLFW_X11",}


    filter "configurations:Debug"
        defines { "_DEBUG" , "TRACY_ENABLE", "NOMINMAX" }
        editandcontinue "off"
        symbols  "Full"
        optimize "Off"

    filter "configurations:Profile"
        defines { "NDEBUG" , "TRACY_ENABLE", "NOMINMAX" }
        editandcontinue "off"
        runtime "Release"
        symbols  "Full"
        --floatingpoint "fast"
        optimize "Speed"

    filter "configurations:Release"
        defines { "NDEBUG", "NOMINMAX" }
        editandcontinue "off"
        runtime "Release"
        symbols  "Full"
        --floatingpoint "fast"
        optimize "Speed"

    filter("files:**.hlsl")
        excludefrombuild "On"

    filter "files:**.natvis"
        buildaction "Natvis"
