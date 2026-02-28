require "ecc/ecc"

workspace "ScadaBackup"
    configurations { "Debug", "Profile", "Release" }
    platforms { "x64" }
    --platforms { "x64", "Win32" }
    staticruntime "On"
    runtime "Debug"

project "ScadaBackup"
    --kind "WindowedApp"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    targetdir "build/"
    targetname "ScadaBackup_%{cfg.system}_%{cfg.platform}_%{cfg.buildcfg}"
    objdir "build/obj/%{cfg.platform}/%{cfg.buildcfg}"

    editandcontinue "Off"
    usefullpaths "On"
    --usestandardpreprocessor 'On'
    --characterset "ASCII"

    --Flags
    multiprocessorcompile "On"
    enablepch "Off"
    fatalwarnings { "All" }

    dependson {
        "libarchive",
        "contrib",
    }

    links {
        "libarchive",
        "contrib",
    }

    --libdirs { }

    includedirs {
        "contrib",
        "contrib/ImGui",
        "contrib/SDL2/include",
        "contrib/tracy/public/tracy",
        "contrib/glfw/include",
        "resources",
    }
    defines {
        "_CRT_SECURE_NO_WARNINGS",
        "LIBARCHIVE_STATIC",
    }
    files {
        "src/**",
        "contrib/ImGui/*.h",
        "contrib/ImGui/backends/imgui_impl_opengl3.*h",
        "contrib/ImGui/backends/imgui_impl_glfw.*h",
        "contrib/json.hpp",
        "contrib/stb/**.h",
        "contrib/libarchive/*.h",
        "resources/**",

        "contrib/glfw/src/internal.h",
        "contrib/glfw/src/platform.h",
        "contrib/glfw/src/mappings.h",
        "contrib/glfw/src/null_platform.h",
        "contrib/glfw/src/null_joystick.h",
    }

    filter "system:Windows"
        system "windows"
        defines {
        "WIN32",
        "_GLFW_WIN32",}

        files { }

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


project "contrib"
    kind "StaticLib"
    language "C++"
    staticruntime "On"
    --cdialect "C99"
    targetdir "build/"
    targetname "contrib_%{cfg.system}_%{cfg.platform}_%{cfg.buildcfg}"
    --targetname "libarchive"
    objdir "build/obj/%{cfg.platform}/%{cfg.buildcfg}"
    editandcontinue "Off"
    usefullpaths "On"

    multiprocessorcompile "On"
    enablepch "Off"
    fatalwarnings { "All" }

    links {
        "OpenGL32",
        "libarchive"
    }

    warnings ("Default");

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
    defines {
        "_CRT_SECURE_NO_WARNINGS",
    }
    files {
        "contrib/tracy/public/TracyClient.cpp",
        "contrib/ImGui/*.cpp",
        "contrib/ImGui/*.h",
        "contrib/ImGui/backends/imgui_impl_opengl3.*",
        "contrib/ImGui/backends/imgui_impl_glfw.*",
        "contrib/json.hpp",
        "contrib/stb/**",
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


project "libarchive"
    kind "StaticLib"
    language "C"
    staticruntime "On"
    --cdialect "C99"
    targetdir "build/"
    targetname "libarchive_%{cfg.system}_%{cfg.platform}_%{cfg.buildcfg}"
    --targetname "libarchive"
    objdir "build/obj/%{cfg.platform}/%{cfg.buildcfg}"
    editandcontinue "Off"
    usefullpaths "On"

    multiprocessorcompile "On"
    enablepch "Off"
    --fatalwarnings { "None" }

    links {
        "archive",
        "zlib",
        "lzma",
        "bz2",
        "zstd",
        "lz4",
        "libcrypto",
        "libssl",
        "xmllite",
        "bcrypt",
        "crypt32",
        "ws2_32",
        "advapi32",
    }

    --warnings ("Default");
    warnings ("Off");

    libdirs {
        "contrib/libarchive/contrib/**",
        "contrib/libarchive_dep/openssl",
        "contrib/libarchive_dep/lzma",
        "contrib/libarchive_dep/lib-%{cfg.platform}-%{cfg.system}-static",
    }

    includedirs {
        --"contrib/libarchive/contrib/**",
        --"contrib/libarchive/libarchive/**",
        "contrib/libarchive",
        "contrib/libarchive/libarchive",
        "contrib/libarchive/contrib",
        "contrib/libarchive_dep/",
    }
    defines {
        --"_CRT_SECURE_NO_WARNINGS",
        "LIBARCHIVE_STATIC",
        "LIB_DLL",
        "USE_BZIP2_DLL",
        "HAVE_CONFIG_H",
        "_CRT_SECURE_NO_DEPRECATE",
        "ARCHIVE_STATIC",
        --"PLATFORM_CONFIG_H=<contrib/libarchive/libarchive/config.h>"
        "PLATFORM_CONFIG_H=<config.h>",
        "NODEFAULTLIB",
        "__LIBARCHIVE_BUILD",
    }
    files {
        "contrib/libarchive/libarchive/**",
        "contrib/libarchive/libarchive/config.h",
        --"contrib/libarchive_dep/**",
        "contrib/libarchive_dep/openssl/**",
        "contrib/libarchive_dep/lzma/**",
        --"contrib/libarchive_dep/lib/**",
        "contrib/libarchive_dep/lib-%{cfg.platform}-%{cfg.system}-static/**",
    }
    removefiles {
        --"contrib/libarchive/contrib/**",
        "contrib/libarchive/libarchive/*_posix.c",
        "contrib/libarchive/libarchive/filter_fork_posix.c",

        "**/*_posix.c",
        "**/*_darwin.c",
        "**/*_freebsd.c",
        "**/*_linux.c",
        "**/*_sunos.c",
    }

    filter "system:Windows"
        system "windows"
        defines { "WIN32" }


    filter "system:Unix"
        system "linux"
        defines { "LINUX", }


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

