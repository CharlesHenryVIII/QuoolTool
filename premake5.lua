require "ecc/ecc"

--local windows_version = "0x0400" -- Windows NT 4.0
--local windows_version = "0x0500" -- Windows 2000
--local windows_version = "0x0501" -- Windows XP
--local windows_version = "0x0502" -- Windows Server 2003
--local windows_version = "0x0600" -- Windows Vista
--local windows_version = "0x0600" -- Windows Vista
--local windows_version = "0x0600" -- Windows Server 2008
--local windows_version = "0x0600" -- Windows Vista
--local windows_version = "0x0601" -- Windows 7
local windows_version = "0x0602" -- Windows 8
--local windows_version = "0x0603" -- Windows 8.1
--local windows_version = "0x0A00" -- Windows 10
--local windows_version = "0x0A00" -- Windows 10

local windows_defines = {
    "WIN32",
    "WINVER=" .. windows_version,
    "_WIN32_WINNT=" .. windows_version,
}

workspace "QuoolTool"
    configurations { "Debug", "Profile", "Release" }
    platforms { "x64" }
    --platforms { "x64", "Win32" }
    staticruntime "on"
    runtime "Debug"
    startproject "QuoolTool"
    --toolset "v141_xp"

project "QuoolTool"
    kind "WindowedApp"
    --kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    targetdir "build/"
    targetname "QuoolTool_%{cfg.system}_%{cfg.platform}_%{cfg.buildcfg}"
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
        "curl-lib",
        "SDL3-lib",
    }

    links {
        "libarchive",
        "contrib",
        "curl-lib",
        "SDL3-lib",
    }

    includedirs {
        "contrib",
        "contrib/ImGui",
        "contrib/SDL3/include",
        "contrib/tracy/public/tracy",
        "contrib/curl/include",
        "contrib/libxlsxwriter/include",
        "resources",
    }
    defines {
        "_CRT_SECURE_NO_WARNINGS",
        "LIBARCHIVE_STATIC",
        "IMGUI_DEFINE_MATH_OPERATORS",
        "CURL_STATICLIB",
    }
    files {
        "src/**",
        "contrib/ImGui/*.h",
        "contrib/ImGui/backends/imgui_impl_sdl3.*",
        "contrib/ImGui/backends/imgui_impl_sdlrenderer3.*",
        "contrib/json.hpp",
        "contrib/stb/**.h",
        "contrib/libarchive/*.h",
        "resources/**",
    }

    filter "system:Windows"
        system "windows"
        defines {
            windows_defines,
        }

        files { }

    filter "system:Unix"
        system "linux"
        defines {
        "LINUX", }


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
        "libarchive",
        "curl-lib",
        "zlib",
    }

    warnings ("Default");

    libdirs {
        "contrib/ImGui",
        "contrib/tracy",
        "contrib/libarchive_dep/lib-%{cfg.platform}-%{cfg.system}-static",
    }

    includedirs {
        "contrib",
        "contrib/ImGui",
        "contrib/SDL3/include",
        "contrib/tracy/public/tracy",
        "contrib/curl/include",
        "contrib/libxlsxwriter/include",
        "contrib/libxlsxwriter/third_party/*",
        "contrib/libarchive_dep",
    }
    defines {
        "_CRT_SECURE_NO_WARNINGS",
        "USE_STATIC_MSVC_RUNTIME",
        "IOWIN32_USING_WINRT_API=0",
    }
    files {
        "contrib/tracy/public/TracyClient.cpp",
        "contrib/ImGui/*.cpp",
        "contrib/ImGui/*.h",
        "contrib/ImGui/backends/imgui_impl_sdl3.*",
        "contrib/ImGui/backends/imgui_impl_sdlrenderer3.*",
        "contrib/json.hpp",
        "contrib/stb/**",
        "contrib/libxlsxwriter/src/**",
        "contrib/libxlsxwriter/third_party/minizip/*.c",
        "contrib/libxlsxwriter/third_party/minizip/*.h",
        "contrib/libxlsxwriter/third_party/tmpfileplus/*.c",
        "contrib/libxlsxwriter/third_party/tmpfileplus/*.h",
    }

    removefiles {
        "contrib/libxlsxwriter/third_party/minizip/minizip.c",
        "contrib/libxlsxwriter/third_party/minizip/miniunz.c"
    }

	filter { "options:not zlib-src=none" }
		defines     { 'USE_ZLIB' }

	filter { "options:zlib-src=contrib" }
		includedirs { '../zlib' }


    filter "system:Windows"
        system "windows"
        defines {
            windows_defines,
        }


    filter "system:Unix"
        system "linux"
        defines {
        "LINUX",
        }


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
        --"archive",
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
        "contrib/libarchive_dep/lib-%{cfg.platform}-%{cfg.system}-static", --static
        --"contrib/libarchive_dep/lib-%{cfg.platform}-%{cfg.system}", --dynamic
    }

    includedirs {
        "contrib/libarchive",
        "contrib/libarchive/libarchive",
        "contrib/libarchive/contrib",
        "contrib/libarchive_dep/",
    }
    defines {
        "LIBARCHIVE_STATIC",
        "LIB_DLL",
        "USE_BZIP2_DLL",
        "HAVE_CONFIG_H",
        "_CRT_SECURE_NO_DEPRECATE",
        "ARCHIVE_STATIC",
        "PLATFORM_CONFIG_H=<config.h>",
        "NODEFAULTLIB",
        "__LIBARCHIVE_BUILD",
    }
    files {
        "contrib/libarchive/libarchive/**",
        "contrib/libarchive/libarchive/config.h",
        "contrib/libarchive_dep/openssl/**",
        "contrib/libarchive_dep/lzma/**",
        "contrib/libarchive_dep/lib-%{cfg.platform}-%{cfg.system}-static/**",
    }
    removefiles {
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
        defines {
            windows_defines,
        }


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


project "curl-lib"
    kind "StaticLib"
    language "C"
    --cdialect "C99"
    targetdir "build/"
    targetname "curl_%{cfg.system}_%{cfg.platform}_%{cfg.buildcfg}"
    objdir "build/obj/%{cfg.platform}/%{cfg.buildcfg}"
    editandcontinue "Off"
    usefullpaths "On"

    multiprocessorcompile "On"
    enablepch "Off"
    --fatalwarnings { "All" }
    warnings "Off";

    externalincludedirs {
        "contrib/curl/include",
    }
    includedirs {
        "contrib/curl/include",
    }
    defines {
        "BUILDING_LIBCURL",
        "CURL_STATICLIB",
        "HTTP_ONLY",
    }
    files {
        "contrib/curl/include/**.c",
        "contrib/curl/include/**.h",
        "contrib/curl/lib/**.c",
        "contrib/curl/lib/**.h",
    }

    filter "system:Windows"
        system "windows"
        defines {
            windows_defines,
        }


	filter { "options:not zlib-src=none" }
		defines     { 'USE_ZLIB' }

	filter { "options:zlib-src=contrib" }
		includedirs { '../zlib' }

	filter { "system:windows" }
		defines { "USE_SCHANNEL", "USE_WINDOWS_SSPI" }
		links { "crypt32", "bcrypt", "secur32", "ws2_32" }

	filter { "system:macosx" }
		defines { "USE_SECTRANSP" }

	filter { "system:not windows", "system:not macosx" }
		defines { "USE_MBEDTLS" }

	filter { "system:linux or toolset:cosmocc"}
		defines { "_GNU_SOURCE" }

	filter { "system:linux or bsd or solaris or haiku or toolset:cosmocc" }
		defines { "CURL_HIDDEN_SYMBOLS" }

		-- find the location of the ca bundle
		local ca = nil
		for _, f in ipairs {
			"/etc/ssl/certs/ca-certificates.crt",
			"/etc/openssl/certs/ca-certificates.crt",
			"/etc/pki/tls/certs/ca-bundle.crt",
			"/usr/share/ssl/certs/ca-bundle.crt",
			"/usr/local/share/certs/ca-root.crt",
			"/usr/local/share/certs/ca-root-nss.crt",
			"/etc/certs/ca-certificates.crt",
			"/etc/ssl/cert.pem",
			"/etc/ssl/cacert.pem",
			"/boot/system/data/ssl/CARootCertificates.pem" } do
			if os.isfile(f) then
				ca = f
				break
			end
		end
		if ca then
			defines { 'CURL_CA_BUNDLE="' .. ca .. '"', 'CURL_CA_PATH="' .. path.getdirectory(ca) .. '"' }
		end

project "SDL3-lib"
    kind "StaticLib"
    language "C"

    targetdir "build/"
    targetname "SDL3_%{cfg.system}_%{cfg.platform}_%{cfg.buildcfg}"
    objdir "build/obj/%{cfg.platform}/%{cfg.buildcfg}"

    multiprocessorcompile "On"
    enablepch "Off"
    warnings "Off"

    includedirs {
        "contrib/SDL3/include",
        "contrib/SDL3/include/build_config",
        "contrib/SDL3/src",
    }
    defines {
        "DSDL_FORCE_STATIC_VCRT=ON",
    }

    filter "system:Windows"
        system "windows"

        defines {
            windows_defines,
        }

        links {
            "setupapi",
            "winmm",
            "imm32",
            "version",
            "ole32",
            "oleaut32",
            "shell32",
            "advapi32",
            "user32",
            "gdi32",
            "ws2_32",
            "dxguid",
        }

        files {
            "contrib/SDL3/src/*.c",
            --
            "contrib/SDL3/src/atomic/*",
            "contrib/SDL3/src/audio/*",
            "contrib/SDL3/src/audio/directsound/*",
            "contrib/SDL3/src/audio/disk/*",
            "contrib/SDL3/src/audio/dummy/*",
            "contrib/SDL3/src/audio/wasapi/*",
            "contrib/SDL3/src/camera/*",
            "contrib/SDL3/src/camera/dummy/*",
            "contrib/SDL3/src/camera/mediafoundation/*",
            "contrib/SDL3/src/core/*",
            "contrib/SDL3/src/core/windows/*",
            "contrib/SDL3/src/cpuinfo/*",
            "contrib/SDL3/src/dialog/*",
            "contrib/SDL3/src/dialog/windows/*",
            "contrib/SDL3/src/dynapi/*",
            "contrib/SDL3/src/events/*",
            "contrib/SDL3/src/filesystem/*",
            "contrib/SDL3/src/filesystem/windows/*",
            "contrib/SDL3/src/gamepad/*.c",
            "contrib/SDL3/src/gpu/*.c",             "contrib/SDL3/src/gpu/*.h",
            "contrib/SDL3/src/gpu/d3d12/*.c",       "contrib/SDL3/src/gpu/d3d12/*.h",
            "contrib/SDL3/src/gpu/vulkan/*.c",      "contrib/SDL3/src/gpu/vulkan/*.h",
            "contrib/SDL3/src/gpu/xr/*.c",          "contrib/SDL3/src/gpu/xr/*.h",
            "contrib/SDL3/src/haptic/*",
            "contrib/SDL3/src/haptic/hidapi/*",
            "contrib/SDL3/src/haptic/windows/*",
            "contrib/SDL3/src/hidapi/*",
            "contrib/SDL3/src/io/*",
            "contrib/SDL3/src/io/generic/*",
            "contrib/SDL3/src/io/windows/*",
            "contrib/SDL3/src/joystick/*",
            "contrib/SDL3/src/joystick/gdk/*",
            "contrib/SDL3/src/joystick/hidapi/*",
            "contrib/SDL3/src/joystick/virtual/*",
            "contrib/SDL3/src/joystick/windows/*",
            "contrib/SDL3/src/loadso/windows/*",
            "contrib/SDL3/src/locale/*",
            "contrib/SDL3/src/locale/windows/*",
            "contrib/SDL3/src/main/*",
            "contrib/SDL3/src/main/generic/*",
            "contrib/SDL3/src/main/windows/*",
            "contrib/SDL3/src/misc/*",
            "contrib/SDL3/src/misc/windows/*",
            "contrib/SDL3/src/power/*",
            "contrib/SDL3/src/power/*.c",
            "contrib/SDL3/src/power/windows/*",
            "contrib/SDL3/src/process/*",
            "contrib/SDL3/src/process/windows/*",
            "contrib/SDL3/src/render/**.c",         "contrib/SDL3/src/render/**.h",
            "contrib/SDL3/src/sensor/*",
            "contrib/SDL3/src/sensor/windows/*",
            "contrib/SDL3/src/stdlib/*",
            "contrib/SDL3/src/storage/*",
            "contrib/SDL3/src/storage/generic/*",
            "contrib/SDL3/src/storage/steam/*",
            "contrib/SDL3/src/thread/*",
            "contrib/SDL3/src/thread/generic/*",
            "contrib/SDL3/src/thread/windows/*",
            "contrib/SDL3/src/time/*",
            "contrib/SDL3/src/time/windows/*",
            "contrib/SDL3/src/timer/*",
            "contrib/SDL3/src/timer/*.c",
            "contrib/SDL3/src/timer/windows/*",
            "contrib/SDL3/src/tray/*",
            "contrib/SDL3/src/tray/windows/*",
            "contrib/SDL3/src/video/*",
            "contrib/SDL3/src/video/dummy/*",
            "contrib/SDL3/src/video/offscreen/*",
            "contrib/SDL3/src/video/windows/*",
            "contrib/SDL3/src/video/yuv2rgb/*",
        }

        defines {
            "SDL_STATIC_LIB",
            "SDL_BUILDING_LIBRARY",
            --
            "SDL_VIDEO_DRIVER_WINDOWS",
            "SDL_FILESYSTEM_WINDOWS",
            "SDL_LOADSO_WINDOWS",
            "SDL_THREAD_WINDOWS",
            "SDL_TIMER_WINDOWS",
            "SDL_POWER_WINDOWS",
            "SDL_JOYSTICK_WINDOWS",
            "SDL_GAMEINPUT_WINDOWS",
            "SDL_HAPTIC_WINDOWS",
            --
            "SDL_DISABLE_ALSA",
            "SDL_DISABLE_PULSEAUDIO",
            "SDL_DISABLE_X11",
            "SDL_DISABLE_WAYLAND",
        }

        removefiles {
            "contrib/SDL3/src/*linux*",
            "contrib/SDL3/src/*posix*",
            "contrib/SDL3/src/*wayland*",
            "contrib/SDL3/src/*x11*",
            "contrib/SDL3/src/*cocoa*",
            "contrib/SDL3/src/*unix*",
            "contrib/SDL3/src/*ps2*",
        }

    filter "configurations:Debug"
        symbols "Full"
        optimize "Off"

    filter "configurations:Profile"
        runtime "Release"
        symbols "Full"
        optimize "Speed"

    filter "configurations:Release"
        runtime "Release"
        optimize "Speed"
