require "make/ecc/ecc"

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

local windows_defines = {
    "WIN32",
    "WINVER=" .. windows_version,
    "_WIN32_WINNT=" .. windows_version,
}

local SDL_DIR  = "contrib/SDL3"

function CommonFilters()
    filter "system:Windows"
        system "windows"
        defines { windows_defines, }
        files { }
    --
    filter "system:linux"
        system "linux"
        defines { "LINUX", }
    --
    filter "system:macosx"
        system "macosx"
        defines { "MACOS", }
    --
    filter "configurations:Debug"
        defines { "_DEBUG" , "TRACY_ENABLE", "NOMINMAX" }
        symbols  "Full"
        optimize "Off"
    --
    filter "configurations:Profile"
        defines { "NDEBUG" , "TRACY_ENABLE", "NOMINMAX" }
        runtime "Release"
        symbols  "Full"
        --floatingpoint "fast"
        optimize "Speed"
    --
    filter "configurations:Release"
        defines { "NDEBUG", "NOMINMAX" }
        runtime "Release"
        symbols  "Full"
        --floatingpoint "fast"
        optimize "Speed"
    --
    filter("files:**.hlsl")
        excludefrombuild "On"
    --
    filter "files:**.natvis"
        buildaction "Natvis"
    --
end

workspace "QuoolTool"
    configurations { "Debug", "Profile", "Release" }
    platforms { "x64" }
    --platforms { "x64", "Win32" }
    staticruntime "on"
    runtime "Debug"
    startproject "QuoolTool"
    --toolset "v141_xp"
    multiprocessorcompile "On"
    editandcontinue "Off"
    usefullpaths "On"
    enablepch "Off"
    targetdir "build/"

    filter "system:Windows"
        require("vstudio")
        premake.override(premake.vstudio.vc2010.elements, "globals", function(base, prj)
            local calls = base(prj)
            table.insert(calls, function(prj)
                premake.w('<VcpkgEnabled>false</VcpkgEnabled>')
            end)
            return calls
        end)


project "QuoolTool"
    kind "WindowedApp" --kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    targetname "QuoolTool_%{cfg.system}_%{cfg.platform}_%{cfg.buildcfg}"
    objdir "build/obj/%{cfg.platform}/%{cfg.buildcfg}"
    filter { "toolset:msc" }
        fatalwarnings { "All" }
    filter { "toolset:clang" }
        fatalwarnings { "All" }
        disablewarnings {
            "missing-declarations",
            "unused-variable",
            "unused-function",
            "unused-value",
            "c23-extensions",
    }
    filter {}

    dependson {
        "libarchive",
        "contrib",
        "curl-lib",
        "SDL3-lib",
    }

    libdirs {
        "build/",
    }

    includedirs {
        "contrib",
        "contrib/CashUtil",
        "contrib/ImGui",
        "contrib/bx/include",
        "contrib/curl/include",
        "contrib/libxlsxwriter/include",
        "contrib/pugixml/src",
        "contrib/sokol",
        "contrib/sokol/util",
        "contrib/tracy/public/tracy",
        "resources",
        path.join(SDL_DIR, "include"),
    }
    defines {
        "_CRT_SECURE_NO_WARNINGS",
        "LIBARCHIVE_STATIC",
        "IMGUI_DEFINE_MATH_OPERATORS",
        "CURL_STATICLIB",
        "SOKOL_IMGUI_NO_SOKOL_APP",
    }
    files {
        "contrib/CashUtil/CashUtil.h",
        "contrib/CashUtil/include/*.cpp",
        "contrib/CashUtil/include/*.h",
        "contrib/CashUtil/include/Shaders/*",
        "contrib/ImGui/*.h",
        --
        "contrib/ImGui/backends/imgui_impl_sdl3.*",
        "contrib/sokol/util/sokol_imgui.h",
        "contrib/sokol/sokol_gfx.h",
        --
        "contrib/json.hpp",
        "contrib/libarchive/*.h",
        "contrib/pugixml/src/*.hpp",
        "contrib/stb/**.h",
        "resources/**",
        "src/**",
    }
    filter("files:**.glsl")
        excludefrombuild ("On")


    filter { "system:Windows" }
        links {
            "OpenGL32",
            "libarchive",
            "contrib",
            "curl-lib",
            "SDL3-lib",
            "iphlpapi",
            "ws2_32",
            "wbemuuid",
            "gdi32",
            "kernel32",
            "psapi",
            "setupapi",
            "winmm",
            "user32",
            "imm32",
            "version",
            "ole32",
            "oleaut32",
            "shell32",
            "advapi32",
            "dxguid",
            "d3d11",
            "dxgi",
        }
        files {
            "contrib/CashUtil/include/Windows/**.cpp",
            "contrib/CashUtil/include/Windows/**.h",
            "resources/**",
        }

    filter { "system:linux" }
        links {
            "dl",
            "GL",
            "pthread",
            "X11",
        }
        files {
            "contrib/CashUtil/include/Linux/**.cpp",
            "contrib/CashUtil/include/Linux/**.h",
        }

    filter "system:macosx"
		links {
            "QuartzCore.framework",
            "Metal.framework",
            "Cocoa.framework",
            "IOKit.framework",
            "CoreVideo.framework"
        }
    filter {}

    CommonFilters()


project "Packager"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    targetdir "build/Packager"
    targetname "QuoolToolPackager_%{cfg.system}_%{cfg.platform}_%{cfg.buildcfg}"
    objdir "build/Packager/obj/%{cfg.platform}/%{cfg.buildcfg}"

    --targetname "QuoolTool_%{cfg.system}_%{cfg.platform}_%{cfg.buildcfg}"
    --objdir "build/obj/%{cfg.platform}/%{cfg.buildcfg}"

    --usestandardpreprocessor 'On'
    --characterset "ASCII"

    --Flags
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
        "iphlpapi",
        "ws2_32",
    }

    libdirs {
        "Packager",
    }

    includedirs {
        "contrib",
        "contrib/ImGui",
        path.join(SDL_DIR, "include"),
        "contrib/tracy/public/tracy",
        "contrib/curl/include",
        "contrib/libxlsxwriter/include",
        "contrib/pugixml/src",
        "contrib/CashUtil",
        "resources",
        --"src",
    }
    defines {
        "_CRT_SECURE_NO_WARNINGS",
        "LIBARCHIVE_STATIC",
        "IMGUI_DEFINE_MATH_OPERATORS",
        "CURL_STATICLIB",
        "SOKOL_IMGUI_NO_SOKOL_APP",
    }
    files {
        "src/**",
        "contrib/CashUtil/CashUtil.h",
        "contrib/CashUtil/include/*.cpp",
        "contrib/CashUtil/include/*.h",
        "contrib/CashUtil/include/Shaders/*",
        "packager/packager.cpp",
        "contrib/ImGui/*.h",
        "contrib/ImGui/backends/imgui_impl_sdl3.*",
        "contrib/json.hpp",
        "contrib/stb/**.h",
        "contrib/libarchive/*.h",
        "contrib/pugixml/src/*.hpp",
        "resources/**",
    }
    filter("files:**.glsl")
        excludefrombuild ("On")

    removefiles {
        "src/main.cpp",
    }

    filter { "system:Windows" }
        links {
            "OpenGL32",
            "libarchive",
            "contrib",
            "curl-lib",
            "SDL3-lib",
            "iphlpapi",
            "ws2_32",
            "wbemuuid",
            "setupapi",
            "winmm",
            "user32",
            "imm32",
            "version",
            "ole32",
            "oleaut32",
            "shell32",
            "advapi32",
            "dxguid",
        }
        files {
            "contrib/CashUtil/include/Windows/**.cpp",
            "contrib/CashUtil/include/Windows/**.h",
            "resources/**",
        }

    filter { "system:linux" }
        links {
            "GL",
        }
        files {
            "contrib/CashUtil/include/Linux/**.cpp",
            "contrib/CashUtil/include/Linux/**.h",
        }

    filter "system:macosx"

    CommonFilters()


project "contrib"
    kind "StaticLib"
    language "C++"
    --cdialect "C99"
    targetname "contrib_%{cfg.system}_%{cfg.platform}_%{cfg.buildcfg}"
    --targetname "libarchive"
    objdir "build/obj/%{cfg.platform}/%{cfg.buildcfg}"

    fatalwarnings { "All" }

    links {
        "libarchive",
        "curl-lib",
        "zlib",
        "wbemuuid",
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
        "contrib/curl/include",
        "contrib/libarchive_dep",
        "contrib/libxlsxwriter/include",
        "contrib/libxlsxwriter/third_party/*",
        "contrib/tracy/public/tracy",
        path.join(SDL_DIR, "include"),
    }
    defines {
        "_CRT_SECURE_NO_WARNINGS",
        "USE_STATIC_MSVC_RUNTIME",
        "IOWIN32_USING_WINRT_API=0",
    }
    files {
        "contrib/ImGui/*.cpp",
        "contrib/ImGui/*.h",
        --
        "contrib/ImGui/backends/imgui_impl_sdl3.*",
        "contrib/tracy/public/TracyClient.cpp",
        "contrib/json.hpp",
        "contrib/libxlsxwriter/src/**",
        "contrib/libxlsxwriter/third_party/minizip/*.c",
        "contrib/libxlsxwriter/third_party/minizip/*.h",
        "contrib/libxlsxwriter/third_party/tmpfileplus/*.c",
        "contrib/libxlsxwriter/third_party/tmpfileplus/*.h",
        "contrib/pugixml/src/*.cpp",
        "contrib/pugixml/src/*.hpp",
        "contrib/stb/**",
    }

    removefiles {
        "contrib/libxlsxwriter/third_party/minizip/minizip.c",
        "contrib/libxlsxwriter/third_party/minizip/miniunz.c"
    }

	filter { "options:not zlib-src=none" }
		defines     { 'USE_ZLIB' }

	filter { "options:zlib-src=contrib" }
		includedirs { '../zlib' }

    CommonFilters()


project "libarchive"
    kind "StaticLib"
    language "C"
    --cdialect "C99"
    targetname "libarchive_%{cfg.system}_%{cfg.platform}_%{cfg.buildcfg}"
    --targetname "libarchive"
    objdir "build/obj/%{cfg.platform}/%{cfg.buildcfg}"

    --fatalwarnings { "None" }

    links {
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
        --"LIB_DLL",
        --"USE_BZIP2_DLL",
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

    CommonFilters()


project "curl-lib"
    kind "StaticLib"
    language "C"
    --cdialect "C99"
    targetname "curl_%{cfg.system}_%{cfg.platform}_%{cfg.buildcfg}"
    objdir "build/obj/%{cfg.platform}/%{cfg.buildcfg}"

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

    targetname "SDL3_%{cfg.system}_%{cfg.platform}_%{cfg.buildcfg}"
    objdir "build/obj/%{cfg.platform}/%{cfg.buildcfg}"

    warnings "Off"

    includedirs {
        path.join(SDL_DIR, "include"),
        path.join(SDL_DIR, "include/build_config"),
        path.join(SDL_DIR, "src"),
    }
    defines {
        "DSDL_FORCE_STATIC_VCRT=ON",
    }

    filter "system:Windows"
        system "windows"

        links {
            --"setupapi",
            --"winmm",
            "imm32",
            "version",
            "ole32",
            "oleaut32",
            "shell32",
            "advapi32",
            --"user32",
            "gdi32",
            "ws2_32",
            "dxguid",
        }

        files {
            path.join(SDL_DIR, "src/*.c"),
            path.join(SDL_DIR, "src/atomic/*"),
            path.join(SDL_DIR, "src/audio/*"),
            path.join(SDL_DIR, "src/audio/directsound/*"),
            path.join(SDL_DIR, "src/audio/disk/*"),
            path.join(SDL_DIR, "src/audio/dummy/*"),
            path.join(SDL_DIR, "src/audio/wasapi/*"),
            path.join(SDL_DIR, "src/camera/*"),
            path.join(SDL_DIR, "src/camera/dummy/*"),
            path.join(SDL_DIR, "src/camera/mediafoundation/*"),
            path.join(SDL_DIR, "src/core/*"),
            path.join(SDL_DIR, "src/core/windows/*"),
            path.join(SDL_DIR, "src/cpuinfo/*"),
            path.join(SDL_DIR, "src/dialog/*"),
            path.join(SDL_DIR, "src/dialog/windows/*"),
            path.join(SDL_DIR, "src/dynapi/*"),
            path.join(SDL_DIR, "src/events/*"),
            path.join(SDL_DIR, "src/filesystem/*"),
            path.join(SDL_DIR, "src/filesystem/windows/*"),
            path.join(SDL_DIR, "src/gamepad/*.c"),
            path.join(SDL_DIR, "src/gpu/*.c"),             path.join(SDL_DIR, "src/gpu/*.h"),
            path.join(SDL_DIR, "src/gpu/d3d12/*.c"),       path.join(SDL_DIR, "src/gpu/d3d12/*.h"),
            path.join(SDL_DIR, "src/gpu/vulkan/*.c"),      path.join(SDL_DIR, "src/gpu/vulkan/*.h"),
            path.join(SDL_DIR, "src/gpu/xr/*.c"),          path.join(SDL_DIR, "src/gpu/xr/*.h"),
            path.join(SDL_DIR, "src/haptic/*"),
            path.join(SDL_DIR, "src/haptic/hidapi/*"),
            path.join(SDL_DIR, "src/haptic/windows/*"),
            path.join(SDL_DIR, "src/hidapi/*"),
            path.join(SDL_DIR, "src/io/*"),
            path.join(SDL_DIR, "src/io/windows/*"),
            path.join(SDL_DIR, "src/joystick/*"),
            path.join(SDL_DIR, "src/joystick/gdk/*"),
            path.join(SDL_DIR, "src/joystick/hidapi/*"),
            path.join(SDL_DIR, "src/joystick/virtual/*"),
            path.join(SDL_DIR, "src/joystick/windows/*"),
            path.join(SDL_DIR, "src/loadso/windows/*"),
            path.join(SDL_DIR, "src/locale/*"),
            path.join(SDL_DIR, "src/locale/windows/*"),
            path.join(SDL_DIR, "src/main/*"),
            path.join(SDL_DIR, "src/main/windows/*"),
            path.join(SDL_DIR, "src/misc/*"),
            path.join(SDL_DIR, "src/misc/windows/*"),
            path.join(SDL_DIR, "src/power/*"),
            path.join(SDL_DIR, "src/power/*.c"),
            path.join(SDL_DIR, "src/power/windows/*"),
            path.join(SDL_DIR, "src/process/*"),
            path.join(SDL_DIR, "src/process/windows/*"),
            path.join(SDL_DIR, "src/render/**.c"),         path.join(SDL_DIR, "src/render/**.h"),
            path.join(SDL_DIR, "src/sensor/*"),
            path.join(SDL_DIR, "src/sensor/windows/*"),
            path.join(SDL_DIR, "src/stdlib/*"),
            path.join(SDL_DIR, "src/storage/*"),
            path.join(SDL_DIR, "src/storage/generic/*"),
            path.join(SDL_DIR, "src/storage/steam/*"),
            path.join(SDL_DIR, "src/thread/*"),
            path.join(SDL_DIR, "src/thread/windows/*"),
            path.join(SDL_DIR, "src/time/*"),
            path.join(SDL_DIR, "src/time/windows/*"),
            path.join(SDL_DIR, "src/timer/*"),
            path.join(SDL_DIR, "src/timer/*.c"),
            path.join(SDL_DIR, "src/timer/windows/*"),
            path.join(SDL_DIR, "src/tray/*"),
            path.join(SDL_DIR, "src/tray/windows/*"),
            path.join(SDL_DIR, "src/video/*"),
            path.join(SDL_DIR, "src/video/dummy/*"),
            path.join(SDL_DIR, "src/video/offscreen/*"),
            path.join(SDL_DIR, "src/video/windows/*"),
            path.join(SDL_DIR, "src/video/yuv2rgb/*"),
                path.join(SDL_DIR, "src/io/generic/*"),
                path.join(SDL_DIR, "src/main/generic/*"),
                path.join(SDL_DIR, "src/thread/generic/*"),
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
            --path.join(SDL_DIR, "src/video/yuv2rgb/*"),
            path.join(SDL_DIR, "src/*linux*"),
            path.join(SDL_DIR, "src/*posix*"),
            path.join(SDL_DIR, "src/*wayland*"),
            path.join(SDL_DIR, "src/*x11*"),
            path.join(SDL_DIR, "src/*cocoa*"),
            path.join(SDL_DIR, "src/*unix*"),
            path.join(SDL_DIR, "src/*ps2*"),
        }

        --THIS IS NOT COMPLETE
        filter "system:linux"
            files {
                path.join(SDL_DIR, "src/io/generic/*"),
                path.join(SDL_DIR, "src/main/generic/*"),
                path.join(SDL_DIR, "src/thread/generic/*"),
            }

    CommonFilters()
