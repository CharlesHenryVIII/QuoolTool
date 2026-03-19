require "ecc/ecc"

workspace "QuoolTool"
    configurations { "Debug", "Profile", "Release" }
    platforms { "x64" }
    --platforms { "x64", "Win32" }
    staticruntime "On"
    runtime "Debug"
    startproject "QuoolTool"

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

    --libdirs { }

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
        "WIN32",
        "WINVER=0x0601",
        "_WIN32_WINNT=0x0601", }

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

	filter { "options:not zlib-src=none" }
		defines     { 'USE_ZLIB' }

	filter { "options:zlib-src=contrib" }
		includedirs { '../zlib' }


    filter "system:Windows"
        system "windows"
        defines {
        "WIN32",
        "WINVER=0x0601",
        "_WIN32_WINNT=0x0601", }


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
        defines {
        "WIN32",
        "WINVER=0x0601",
        "_WIN32_WINNT=0x0601", }


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
    staticruntime "On"
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
        "WIN32",
        "WINVER=0x0601",
        "_WIN32_WINNT=0x0601", }


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
    staticruntime "On"

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

    filter "system:Windows"
        system "windows"

        defines {
            "WIN32",
            "DSDL_FORCE_STATIC_VCRT=ON",
            "WINVER=0x0601",
            "_WIN32_WINNT=0x0601",
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
    -- Core
    "contrib/SDL3/src/*.c",

    -- Joystick/Gamepad (imgui uses this!)
    "contrib/SDL3/src/gamepad/*.c",

    -- Threading / Timer / etc
    "contrib/SDL3/src/timer/*.c",
    "contrib/SDL3/src/timer/windows/*.c",

    -- Loadso
    "contrib/SDL3/src/loadso/windows/*.c",

    -- Power
    "contrib/SDL3/src/power/*.c",
    "contrib/SDL3/src/power/windows/*.c",

    -- Lib
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

    "contrib/SDL3/src/gpu/d3d12/*.h",
    "contrib/SDL3/src/gpu/d3d12/*.c",
    "contrib/SDL3/src/gpu/vulkan/*.h",
    "contrib/SDL3/src/gpu/vulkan/*.c",
    "contrib/SDL3/src/gpu/xr/*.h",
    "contrib/SDL3/src/gpu/xr/*.c",
    "contrib/SDL3/src/gpu/*.h",
    "contrib/SDL3/src/gpu/*.c",

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
    "contrib/SDL3/src/power/windows/*",
    "contrib/SDL3/src/process/*",
    "contrib/SDL3/src/process/windows/*",

    "contrib/SDL3/src/render/**.c",
    "contrib/SDL3/src/render/**.h",
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
    "contrib/SDL3/src/timer/windows/*",
    "contrib/SDL3/src/tray/*",
    "contrib/SDL3/src/tray/windows/*",

    "contrib/SDL3/src/video/*",
    "contrib/SDL3/src/video/dummy/*",
    "contrib/SDL3/src/video/offscreen/*",
    "contrib/SDL3/src/video/windows/*",
    "contrib/SDL3/src/video/yuv2rgb/*",

    --"contrib/SDL3/src/asyncio/*.c",
    --"contrib/SDL3/src/asyncio/windows/*.c",
    --"contrib/SDL3/src/atomic/*.c",

    --"contrib/SDL3/src/gameinput/*.c",
    --"contrib/SDL3/src/gameinput/windows/*.c",


            --"contrib/SDL3/include/SDL3/SDL.h",
            --"contrib/SDL3/include/SDL3/SDL_assert.h",
            --"contrib/SDL3/include/SDL3/SDL_asyncio.h",
            --"contrib/SDL3/include/SDL3/SDL_atomic.h",
            --"contrib/SDL3/include/SDL3/SDL_audio.h",
            --"contrib/SDL3/include/SDL3/SDL_begin_code.h",
            --"contrib/SDL3/include/SDL3/SDL_bits.h",
            --"contrib/SDL3/include/SDL3/SDL_blendmode.h",
            --"contrib/SDL3/include/SDL3/SDL_camera.h",
            --"contrib/SDL3/include/SDL3/SDL_clipboard.h",
            --"contrib/SDL3/include/SDL3/SDL_close_code.h",
            --"contrib/SDL3/include/SDL3/SDL_copying.h",
            --"contrib/SDL3/include/SDL3/SDL_cpuinfo.h",
            --"contrib/SDL3/include/SDL3/SDL_dialog.h",
            --"contrib/SDL3/include/SDL3/SDL_dlopennote.h",
            --"contrib/SDL3/include/SDL3/SDL_egl.h",
            --"contrib/SDL3/include/SDL3/SDL_endian.h",
            --"contrib/SDL3/include/SDL3/SDL_error.h",
            --"contrib/SDL3/include/SDL3/SDL_events.h",
            --"contrib/SDL3/include/SDL3/SDL_filesystem.h",
            --"contrib/SDL3/include/SDL3/SDL_gamepad.h",
            --"contrib/SDL3/include/SDL3/SDL_gpu.h",
            --"contrib/SDL3/include/SDL3/SDL_guid.h",
            --"contrib/SDL3/include/SDL3/SDL_haptic.h",
            --"contrib/SDL3/include/SDL3/SDL_hidapi.h",
            --"contrib/SDL3/include/SDL3/SDL_hints.h",
            --"contrib/SDL3/include/SDL3/SDL_init.h",
            --"contrib/SDL3/include/SDL3/SDL_intrin.h",
            --"contrib/SDL3/include/SDL3/SDL_iostream.h",
            --"contrib/SDL3/include/SDL3/SDL_joystick.h",
            --"contrib/SDL3/include/SDL3/SDL_keyboard.h",
            --"contrib/SDL3/include/SDL3/SDL_keycode.h",
            --"contrib/SDL3/include/SDL3/SDL_loadso.h",
            --"contrib/SDL3/include/SDL3/SDL_locale.h",
            --"contrib/SDL3/include/SDL3/SDL_log.h",
            --"contrib/SDL3/include/SDL3/SDL_main.h",
            --"contrib/SDL3/include/SDL3/SDL_main_impl.h",
            --"contrib/SDL3/include/SDL3/SDL_messagebox.h",
            --"contrib/SDL3/include/SDL3/SDL_misc.h",
            --"contrib/SDL3/include/SDL3/SDL_mouse.h",
            --"contrib/SDL3/include/SDL3/SDL_mutex.h",
            --"contrib/SDL3/include/SDL3/SDL_oldnames.h",
            --"contrib/SDL3/include/SDL3/SDL_opengl.h",
            --"contrib/SDL3/include/SDL3/SDL_opengl_glext.h",
            --"contrib/SDL3/include/SDL3/SDL_opengles.h",
            --"contrib/SDL3/include/SDL3/SDL_opengles2.h",
            --"contrib/SDL3/include/SDL3/SDL_opengles2_gl2.h",
            --"contrib/SDL3/include/SDL3/SDL_opengles2_gl2ext.h",
            --"contrib/SDL3/include/SDL3/SDL_opengles2_gl2platform.h",
            --"contrib/SDL3/include/SDL3/SDL_opengles2_khrplatform.h",
            --"contrib/SDL3/include/SDL3/SDL_pen.h",
            --"contrib/SDL3/include/SDL3/SDL_pixels.h",
            --"contrib/SDL3/include/SDL3/SDL_platform.h",
            --"contrib/SDL3/include/SDL3/SDL_platform_defines.h",
            --"contrib/SDL3/include/SDL3/SDL_power.h",
            --"contrib/SDL3/include/SDL3/SDL_process.h",
            --"contrib/SDL3/include/SDL3/SDL_properties.h",
            --"contrib/SDL3/include/SDL3/SDL_rect.h",
            --"contrib/SDL3/include/SDL3/SDL_render.h",
            --"contrib/SDL3/include/SDL3/SDL_revision.h",
            --"contrib/SDL3/include/SDL3/SDL_scancode.h",
            --"contrib/SDL3/include/SDL3/SDL_sensor.h",
            --"contrib/SDL3/include/SDL3/SDL_stdinc.h",
            --"contrib/SDL3/include/SDL3/SDL_storage.h",
            --"contrib/SDL3/include/SDL3/SDL_surface.h",
            --"contrib/SDL3/include/SDL3/SDL_system.h",
            --"contrib/SDL3/include/SDL3/SDL_test.h",
            --"contrib/SDL3/include/SDL3/SDL_test_assert.h",
            --"contrib/SDL3/include/SDL3/SDL_test_common.h",
            --"contrib/SDL3/include/SDL3/SDL_test_compare.h",
            --"contrib/SDL3/include/SDL3/SDL_test_crc32.h",
            --"contrib/SDL3/include/SDL3/SDL_test_font.h",
            --"contrib/SDL3/include/SDL3/SDL_test_fuzzer.h",
            --"contrib/SDL3/include/SDL3/SDL_test_harness.h",
            --"contrib/SDL3/include/SDL3/SDL_test_log.h",
            --"contrib/SDL3/include/SDL3/SDL_test_md5.h",
            --"contrib/SDL3/include/SDL3/SDL_test_memory.h",
            --"contrib/SDL3/include/SDL3/SDL_thread.h",
            --"contrib/SDL3/include/SDL3/SDL_time.h",
            --"contrib/SDL3/include/SDL3/SDL_timer.h",
            --"contrib/SDL3/include/SDL3/SDL_touch.h",
            --"contrib/SDL3/include/SDL3/SDL_tray.h",
            --"contrib/SDL3/include/SDL3/SDL_version.h",
            --"contrib/SDL3/include/SDL3/SDL_video.h",
            --"contrib/SDL3/include/SDL3/SDL_vulkan.h",
            --"contrib/SDL3/include/build_config/SDL_build_config.h",
            --"contrib/SDL3/include/build_config/SDL_build_config_minimal.h",
            --"contrib/SDL3/include/build_config/SDL_build_config_windows.h",
            --"contrib/SDL3/include/build_config/SDL_build_config_xbox.h",
            --"contrib/SDL3/src/SDL.c",
            --"contrib/SDL3/src/SDL_assert.c",
            --"contrib/SDL3/src/SDL_assert_c.h",
            --"contrib/SDL3/src/SDL_error.c",
            --"contrib/SDL3/src/SDL_error_c.h",
            --"contrib/SDL3/src/SDL_guid.c",
            --"contrib/SDL3/src/SDL_hashtable.c",
            --"contrib/SDL3/src/SDL_hashtable.h",
            --"contrib/SDL3/src/SDL_hints.c",
            --"contrib/SDL3/src/SDL_hints_c.h",
            --"contrib/SDL3/src/SDL_internal.h",
            --"contrib/SDL3/src/SDL_list.c",
            --"contrib/SDL3/src/SDL_list.h",
            --"contrib/SDL3/src/SDL_log.c",
            --"contrib/SDL3/src/SDL_log_c.h",
            --"contrib/SDL3/src/SDL_properties.c",
            --"contrib/SDL3/src/SDL_properties_c.h",
            --"contrib/SDL3/src/SDL_utils.c",
            --"contrib/SDL3/src/SDL_utils_c.h",
            --"contrib/SDL3/src/atomic/SDL_atomic.c",
            --"contrib/SDL3/src/atomic/SDL_spinlock.c",
            --"contrib/SDL3/src/audio/SDL_audio.c",
            --"contrib/SDL3/src/audio/SDL_audio_c.h",
            --"contrib/SDL3/src/audio/SDL_audio_channel_converters.h",
            --"contrib/SDL3/src/audio/SDL_audiocvt.c",
            --"contrib/SDL3/src/audio/SDL_audiodev.c",
            --"contrib/SDL3/src/audio/SDL_audiodev_c.h",
            --"contrib/SDL3/src/audio/SDL_audioqueue.c",
            --"contrib/SDL3/src/audio/SDL_audioqueue.h",
            --"contrib/SDL3/src/audio/SDL_audioresample.c",
            --"contrib/SDL3/src/audio/SDL_audioresample.h",
            --"contrib/SDL3/src/audio/SDL_audiotypecvt.c",
            --"contrib/SDL3/src/audio/SDL_mixer.c",
            --"contrib/SDL3/src/audio/SDL_sysaudio.h",
            --"contrib/SDL3/src/audio/SDL_wave.c",
            --"contrib/SDL3/src/audio/SDL_wave.h",
            --"contrib/SDL3/src/audio/aaudio/SDL_aaudio.c",
            --"contrib/SDL3/src/audio/aaudio/SDL_aaudio.h",
            --"contrib/SDL3/src/audio/aaudio/SDL_aaudiofuncs.h",
            --"contrib/SDL3/src/audio/alsa/SDL_alsa_audio.c",
            --"contrib/SDL3/src/audio/alsa/SDL_alsa_audio.h",
            --"contrib/SDL3/src/audio/coreaudio/SDL_coreaudio.h",
            --"contrib/SDL3/src/audio/coreaudio/SDL_coreaudio.m",
            --"contrib/SDL3/src/audio/directsound/SDL_directsound.c",
            --"contrib/SDL3/src/audio/directsound/SDL_directsound.h",
            --"contrib/SDL3/src/audio/disk/SDL_diskaudio.c",
            --"contrib/SDL3/src/audio/disk/SDL_diskaudio.h",
            --"contrib/SDL3/src/audio/dsp/SDL_dspaudio.c",
            --"contrib/SDL3/src/audio/dsp/SDL_dspaudio.h",
            --"contrib/SDL3/src/audio/dummy/SDL_dummyaudio.c",
            --"contrib/SDL3/src/audio/dummy/SDL_dummyaudio.h",
            --"contrib/SDL3/src/audio/emscripten/SDL_emscriptenaudio.c",
            --"contrib/SDL3/src/audio/emscripten/SDL_emscriptenaudio.h",
            --"contrib/SDL3/src/audio/haiku/SDL_haikuaudio.cc",
            --"contrib/SDL3/src/audio/haiku/SDL_haikuaudio.h",
            --"contrib/SDL3/src/audio/jack/SDL_jackaudio.c",
            --"contrib/SDL3/src/audio/jack/SDL_jackaudio.h",
            --"contrib/SDL3/src/audio/ngage/SDL_ngageaudio.c",
            --"contrib/SDL3/src/audio/ngage/SDL_ngageaudio.cpp",
            --"contrib/SDL3/src/audio/ngage/SDL_ngageaudio.h",
            --"contrib/SDL3/src/audio/ngage/SDL_ngageaudio.hpp",
            --"contrib/SDL3/src/audio/openslES/SDL_openslES.c",
            --"contrib/SDL3/src/audio/openslES/SDL_openslES.h",
            --"contrib/SDL3/src/audio/pipewire/SDL_pipewire.c",
            --"contrib/SDL3/src/audio/pipewire/SDL_pipewire.h",
            --"contrib/SDL3/src/audio/pulseaudio/SDL_pulseaudio.c",
            --"contrib/SDL3/src/audio/pulseaudio/SDL_pulseaudio.h",
            --"contrib/SDL3/src/audio/qnx/SDL_qsa_audio.c",
            --"contrib/SDL3/src/audio/qnx/SDL_qsa_audio.h",
            --"contrib/SDL3/src/audio/sndio/SDL_sndioaudio.c",
            --"contrib/SDL3/src/audio/sndio/SDL_sndioaudio.h",
            --"contrib/SDL3/src/audio/wasapi/SDL_wasapi.c",
            --"contrib/SDL3/src/audio/wasapi/SDL_wasapi.h",
            --"contrib/SDL3/src/camera/SDL_camera.c",
            --"contrib/SDL3/src/camera/SDL_camera_c.h",
            --"contrib/SDL3/src/camera/SDL_syscamera.h",
            --"contrib/SDL3/src/camera/coremedia/SDL_camera_coremedia.m",
            --"contrib/SDL3/src/camera/dummy/SDL_camera_dummy.c",
            --"contrib/SDL3/src/camera/emscripten/SDL_camera_emscripten.c",
            --"contrib/SDL3/src/camera/mediafoundation/SDL_camera_mediafoundation.c",
            --"contrib/SDL3/src/camera/pipewire/SDL_camera_pipewire.c",
            --"contrib/SDL3/src/camera/v4l2/SDL_camera_v4l2.c",
            --"contrib/SDL3/src/core/*",
            --"contrib/SDL3/src/core/windows/*",
            --"contrib/SDL3/src/cpuinfo/SDL_cpuinfo.c",
            --"contrib/SDL3/src/cpuinfo/SDL_cpuinfo_c.h",
            --"contrib/SDL3/src/dialog/SDL_dialog.c",
            --"contrib/SDL3/src/dialog/SDL_dialog.h",
            --"contrib/SDL3/src/dialog/SDL_dialog_utils.c",
            --"contrib/SDL3/src/dialog/SDL_dialog_utils.h",
            --"contrib/SDL3/src/dialog/dummy/SDL_dummydialog.c",
            --"contrib/SDL3/src/dialog/haiku/SDL_haikudialog.cc",
            --"contrib/SDL3/src/dialog/windows/SDL_windowsdialog.c",
            --"contrib/SDL3/src/dynapi/SDL_dynapi.c",
            --"contrib/SDL3/src/dynapi/SDL_dynapi.h",
            --"contrib/SDL3/src/dynapi/SDL_dynapi.sym",
            --"contrib/SDL3/src/dynapi/SDL_dynapi_overrides.h",
            --"contrib/SDL3/src/dynapi/SDL_dynapi_procs.h",
            --"contrib/SDL3/src/dynapi/SDL_dynapi_unsupported.h",
            --"contrib/SDL3/src/dynapi/gendynapi.py",
            --"contrib/SDL3/src/events/SDL_categories.c",
            --"contrib/SDL3/src/events/SDL_categories_c.h",
            --"contrib/SDL3/src/events/SDL_clipboardevents.c",
            --"contrib/SDL3/src/events/SDL_clipboardevents_c.h",
            --"contrib/SDL3/src/events/SDL_displayevents.c",
            --"contrib/SDL3/src/events/SDL_displayevents_c.h",
            --"contrib/SDL3/src/events/SDL_dropevents.c",
            --"contrib/SDL3/src/events/SDL_dropevents_c.h",
            --"contrib/SDL3/src/events/SDL_events.c",
            --"contrib/SDL3/src/events/SDL_events_c.h",
            --"contrib/SDL3/src/events/SDL_eventwatch.c",
            --"contrib/SDL3/src/events/SDL_eventwatch_c.h",
            --"contrib/SDL3/src/events/SDL_keyboard.c",
            --"contrib/SDL3/src/events/SDL_keyboard_c.h",
            --"contrib/SDL3/src/events/SDL_keymap.c",
            --"contrib/SDL3/src/events/SDL_keymap_c.h",
            --"contrib/SDL3/src/events/SDL_keysym_to_keycode.c",
            --"contrib/SDL3/src/events/SDL_keysym_to_keycode_c.h",
            --"contrib/SDL3/src/events/SDL_keysym_to_scancode.c",
            --"contrib/SDL3/src/events/SDL_keysym_to_scancode_c.h",
            --"contrib/SDL3/src/events/SDL_mouse.c",
            --"contrib/SDL3/src/events/SDL_mouse_c.h",
            --"contrib/SDL3/src/events/SDL_pen.c",
            --"contrib/SDL3/src/events/SDL_pen_c.h",
            --"contrib/SDL3/src/events/SDL_quit.c",
            --"contrib/SDL3/src/events/SDL_scancode_tables.c",
            --"contrib/SDL3/src/events/SDL_scancode_tables_c.h",
            --"contrib/SDL3/src/events/SDL_touch.c",
            --"contrib/SDL3/src/events/SDL_touch_c.h",
            --"contrib/SDL3/src/events/SDL_windowevents.c",
            --"contrib/SDL3/src/events/SDL_windowevents_c.h",
            --"contrib/SDL3/src/events/blank_cursor.h",
            --"contrib/SDL3/src/events/default_cursor.h",
            --"contrib/SDL3/src/events/imKStoUCS.c",
            --"contrib/SDL3/src/events/imKStoUCS.h",
            --"contrib/SDL3/src/events/scancodes_darwin.h",
            --"contrib/SDL3/src/events/scancodes_windows.h",
            --"contrib/SDL3/src/events/scancodes_xfree86.h",
            --"contrib/SDL3/src/filesystem/SDL_filesystem.c",
            --"contrib/SDL3/src/filesystem/SDL_filesystem_c.h",
            --"contrib/SDL3/src/filesystem/SDL_sysfilesystem.h",
            --"contrib/SDL3/src/filesystem/dummy/SDL_sysfilesystem.c",
            --"contrib/SDL3/src/filesystem/dummy/SDL_sysfsops.c",
            --"contrib/SDL3/src/filesystem/emscripten/SDL_sysfilesystem.c",
            --"contrib/SDL3/src/filesystem/haiku/SDL_sysfilesystem.cc",
            --"contrib/SDL3/src/filesystem/ngage/SDL_sysfilesystem.c",
            --"contrib/SDL3/src/filesystem/ngage/SDL_sysfilesystem.cpp",
            --"contrib/SDL3/src/filesystem/posix/SDL_sysfsops.c",
            --"contrib/SDL3/src/filesystem/riscos/SDL_sysfilesystem.c",
            --"contrib/SDL3/src/filesystem/windows/SDL_sysfilesystem.c",
            --"contrib/SDL3/src/filesystem/windows/SDL_sysfsops.c",
            --"contrib/SDL3/src/gpu/SDL_gpu.c",
            --"contrib/SDL3/src/gpu/SDL_sysgpu.h",
            --"contrib/SDL3/src/gpu/d3d12/D3D12_Blit.h",
            --"contrib/SDL3/src/gpu/d3d12/SDL_gpu_d3d12.c",
            --"contrib/SDL3/src/gpu/d3d12/compile_shaders.bat",
            --"contrib/SDL3/src/gpu/d3d12/compile_shaders_xbox.bat",
            --"contrib/SDL3/src/gpu/vulkan/SDL_gpu_vulkan.c",
            --"contrib/SDL3/src/gpu/vulkan/SDL_gpu_vulkan_vkfuncs.h",
            --"contrib/SDL3/src/haptic/SDL_haptic.c",
            --"contrib/SDL3/src/haptic/SDL_haptic_c.h",
            --"contrib/SDL3/src/haptic/SDL_syshaptic.h",
            --"contrib/SDL3/src/haptic/darwin/SDL_syshaptic.c",
            --"contrib/SDL3/src/haptic/darwin/SDL_syshaptic_c.h",
            --"contrib/SDL3/src/haptic/dummy/SDL_syshaptic.c",
            --"contrib/SDL3/src/haptic/hidapi/SDL_hidapihaptic.c",
            --"contrib/SDL3/src/haptic/hidapi/SDL_hidapihaptic.h",
            --"contrib/SDL3/src/haptic/hidapi/SDL_hidapihaptic_c.h",
            --"contrib/SDL3/src/haptic/hidapi/SDL_hidapihaptic_lg4ff.c",
            --"contrib/SDL3/src/haptic/windows/SDL_dinputhaptic.c",
            --"contrib/SDL3/src/haptic/windows/SDL_dinputhaptic_c.h",
            --"contrib/SDL3/src/haptic/windows/SDL_windowshaptic.c",
            --"contrib/SDL3/src/haptic/windows/SDL_windowshaptic_c.h",
            --"contrib/SDL3/src/hidapi/BUILD.autotools.md",
            --"contrib/SDL3/src/hidapi/BUILD.md",
            --"contrib/SDL3/src/hidapi/README.md",
            --"contrib/SDL3/src/hidapi/SDL_hidapi.c",
            --"contrib/SDL3/src/hidapi/SDL_hidapi_c.h",
            --"contrib/SDL3/src/hidapi/SDL_hidapi_libusb.h",
            --"contrib/SDL3/src/hidapi/SDL_hidapi_steamxbox.h",
            --"contrib/SDL3/src/hidapi/SDL_hidapi_windows.h",
            --"contrib/SDL3/src/hidapi/VERSION",
            --"contrib/SDL3/src/hidapi/bootstrap",
            --"contrib/SDL3/src/hidapi/configure.ac",
            --"contrib/SDL3/src/hidapi/dist/hidapi.podspec",
            --"contrib/SDL3/src/hidapi/doxygen/Doxyfile",
            --"contrib/SDL3/src/hidapi/doxygen/main_page.md",
            --"contrib/SDL3/src/hidapi/hidapi/hidapi.h",
            --"contrib/SDL3/src/hidapi/hidtest/test.c",
            --"contrib/SDL3/src/hidapi/libusb/hid.c",
            --"contrib/SDL3/src/hidapi/libusb/hidapi_libusb.h",
            --"contrib/SDL3/src/hidapi/libusb/hidapi_thread_pthread.h",
            --"contrib/SDL3/src/hidapi/libusb/hidapi_thread_sdl.h",
            --"contrib/SDL3/src/hidapi/m4/ax_pthread.m4",
            --"contrib/SDL3/src/hidapi/m4/pkg.m4",
            --"contrib/SDL3/src/hidapi/meson.build",
            --"contrib/SDL3/src/hidapi/pc/hidapi-hidraw.pc.in",
            --"contrib/SDL3/src/hidapi/pc/hidapi-libusb.pc.in",
            --"contrib/SDL3/src/hidapi/pc/hidapi.pc.in",
            --"contrib/SDL3/src/hidapi/subprojects/README.md",
            --"contrib/SDL3/src/hidapi/testgui/TestGUI.app.in/Contents/Info.plist",
            --"contrib/SDL3/src/hidapi/testgui/TestGUI.app.in/Contents/PkgInfo",
            --"contrib/SDL3/src/hidapi/testgui/TestGUI.app.in/Contents/Resources/English.lproj/InfoPlist.strings",
            --"contrib/SDL3/src/hidapi/testgui/TestGUI.app.in/Contents/Resources/Signal11.icns",
            --"contrib/SDL3/src/hidapi/testgui/copy_to_bundle.sh",
            --"contrib/SDL3/src/hidapi/testgui/test.cpp",
            --"contrib/SDL3/src/hidapi/testgui/testgui.vcproj",
            --"contrib/SDL3/src/hidapi/udev/69-hid.rules",
            --"contrib/SDL3/src/hidapi/windows/hid.c",
            --"contrib/SDL3/src/hidapi/windows/hidapi.rc",
            --"contrib/SDL3/src/hidapi/windows/hidapi.vcproj",
            --"contrib/SDL3/src/hidapi/windows/hidapi_cfgmgr32.h",
            --"contrib/SDL3/src/hidapi/windows/hidapi_descriptor_reconstruct.c",
            --"contrib/SDL3/src/hidapi/windows/hidapi_descriptor_reconstruct.h",
            --"contrib/SDL3/src/hidapi/windows/hidapi_hidclass.h",
            --"contrib/SDL3/src/hidapi/windows/hidapi_hidpi.h",
            --"contrib/SDL3/src/hidapi/windows/hidapi_hidsdi.h",
            --"contrib/SDL3/src/hidapi/windows/hidapi_winapi.h",
            --"contrib/SDL3/src/hidapi/windows/hidtest.vcproj",
            --"contrib/SDL3/src/hidapi/windows/pp_data_dump/README.md",
            --"contrib/SDL3/src/hidapi/windows/pp_data_dump/pp_data_dump.c",
            --"contrib/SDL3/src/hidapi/windows/test/data/045E_02FF_0005_0001.pp_data",
            --"contrib/SDL3/src/hidapi/windows/test/data/045E_02FF_0005_0001_expected.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/045E_02FF_0005_0001_real.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/046A_0011_0006_0001.pp_data",
            --"contrib/SDL3/src/hidapi/windows/test/data/046A_0011_0006_0001_expected.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/046A_0011_0006_0001_real.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_0A37_0001_000C.pp_data",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_0A37_0001_000C_expected.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_0A37_0001_000C_real.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_B010_0001_000C.pp_data",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_B010_0001_000C_expected.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_B010_0001_000C_real.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_B010_0001_FF00.pp_data",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_B010_0001_FF00_expected.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_B010_0001_FF00_real.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_B010_0002_0001.pp_data",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_B010_0002_0001_expected.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_B010_0002_0001_real.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_B010_0002_FF00.pp_data",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_B010_0002_FF00_expected.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_B010_0002_FF00_real.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_B010_0006_0001.pp_data",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_B010_0006_0001_expected.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_B010_0006_0001_real.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_C077_0002_0001.pp_data",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_C077_0002_0001_expected.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_C077_0002_0001_real.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_C283_0004_0001.pp_data",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_C283_0004_0001_expected.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_C283_0004_0001_real.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_C52F_0001_000C.pp_data",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_C52F_0001_000C_expected.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_C52F_0001_000C_real.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_C52F_0001_FF00.pp_data",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_C52F_0001_FF00_expected.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_C52F_0001_FF00_real.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_C52F_0002_0001.pp_data",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_C52F_0002_0001_expected.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_C52F_0002_0001_real.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_C52F_0002_FF00.pp_data",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_C52F_0002_FF00_expected.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_C52F_0002_FF00_real.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_C534_0001_000C.pp_data",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_C534_0001_000C_expected.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_C534_0001_000C_real.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_C534_0001_FF00.pp_data",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_C534_0001_FF00_expected.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_C534_0001_FF00_real.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_C534_0002_0001.pp_data",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_C534_0002_0001_expected.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_C534_0002_0001_real.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_C534_0002_FF00.pp_data",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_C534_0002_FF00_expected.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_C534_0002_FF00_real.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_C534_0006_0001.pp_data",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_C534_0006_0001_expected.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_C534_0006_0001_real.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_C534_0080_0001.pp_data",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_C534_0080_0001_expected.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/046D_C534_0080_0001_real.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/047F_C056_0001_000C.pp_data",
            --"contrib/SDL3/src/hidapi/windows/test/data/047F_C056_0001_000C_expected.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/047F_C056_0001_000C_real.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/047F_C056_0003_FFA0.pp_data",
            --"contrib/SDL3/src/hidapi/windows/test/data/047F_C056_0003_FFA0_expected.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/047F_C056_0003_FFA0_real.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/047F_C056_0005_000B.pp_data",
            --"contrib/SDL3/src/hidapi/windows/test/data/047F_C056_0005_000B_expected.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/047F_C056_0005_000B_real.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/17CC_1130_0000_FF01.pp_data",
            --"contrib/SDL3/src/hidapi/windows/test/data/17CC_1130_0000_FF01_expected.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/data/17CC_1130_0000_FF01_real.rpt_desc",
            --"contrib/SDL3/src/hidapi/windows/test/hid_report_reconstructor_test.c",
            --"contrib/SDL3/src/io/SDL_asyncio.c",
            --"contrib/SDL3/src/io/SDL_asyncio_c.h",
            --"contrib/SDL3/src/io/SDL_iostream.c",
            --"contrib/SDL3/src/io/SDL_iostream_c.h",
            --"contrib/SDL3/src/io/SDL_sysasyncio.h",
            --"contrib/SDL3/src/io/generic/SDL_asyncio_generic.c",
            --"contrib/SDL3/src/io/io_uring/SDL_asyncio_liburing.c",
            --"contrib/SDL3/src/io/windows/SDL_asyncio_windows_ioring.c",
            --"contrib/SDL3/src/joystick/SDL_gamepad.c",
            --"contrib/SDL3/src/joystick/SDL_gamepad_c.h",
            --"contrib/SDL3/src/joystick/SDL_gamepad_db.h",
            --"contrib/SDL3/src/joystick/SDL_joystick.c",
            --"contrib/SDL3/src/joystick/SDL_joystick_c.h",
            --"contrib/SDL3/src/joystick/SDL_steam_virtual_gamepad.c",
            --"contrib/SDL3/src/joystick/SDL_steam_virtual_gamepad.h",
            --"contrib/SDL3/src/joystick/SDL_sysjoystick.h",
            --"contrib/SDL3/src/joystick/check_8bitdo.sh",
            --"contrib/SDL3/src/joystick/controller_list.h",
            --"contrib/SDL3/src/joystick/controller_type.c",
            --"contrib/SDL3/src/joystick/controller_type.h",
            --"contrib/SDL3/src/joystick/darwin/SDL_iokitjoystick.c",
            --"contrib/SDL3/src/joystick/darwin/SDL_iokitjoystick_c.h",
            --"contrib/SDL3/src/joystick/dummy/SDL_sysjoystick.c",
            --"contrib/SDL3/src/joystick/emscripten/SDL_sysjoystick.c",
            --"contrib/SDL3/src/joystick/emscripten/SDL_sysjoystick_c.h",
            --"contrib/SDL3/src/joystick/haiku/SDL_haikujoystick.cc",
            --"contrib/SDL3/src/joystick/hidapi/SDL_hidapi_8bitdo.c",
            --"contrib/SDL3/src/joystick/hidapi/SDL_hidapi_combined.c",
            --"contrib/SDL3/src/joystick/hidapi/SDL_hidapi_flydigi.c",
            --"contrib/SDL3/src/joystick/hidapi/SDL_hidapi_flydigi.h",
            --"contrib/SDL3/src/joystick/hidapi/SDL_hidapi_gamecube.c",
            --"contrib/SDL3/src/joystick/hidapi/SDL_hidapi_gip.c",
            --"contrib/SDL3/src/joystick/hidapi/SDL_hidapi_lg4ff.c",
            --"contrib/SDL3/src/joystick/hidapi/SDL_hidapi_luna.c",
            --"contrib/SDL3/src/joystick/hidapi/SDL_hidapi_nintendo.h",
            --"contrib/SDL3/src/joystick/hidapi/SDL_hidapi_ps3.c",
            --"contrib/SDL3/src/joystick/hidapi/SDL_hidapi_ps4.c",
            --"contrib/SDL3/src/joystick/hidapi/SDL_hidapi_ps5.c",
            --"contrib/SDL3/src/joystick/hidapi/SDL_hidapi_rumble.c",
            --"contrib/SDL3/src/joystick/hidapi/SDL_hidapi_rumble.h",
            --"contrib/SDL3/src/joystick/hidapi/SDL_hidapi_shield.c",
            --"contrib/SDL3/src/joystick/hidapi/SDL_hidapi_sinput.c",
            --"contrib/SDL3/src/joystick/hidapi/SDL_hidapi_sinput.h",
            --"contrib/SDL3/src/joystick/hidapi/SDL_hidapi_stadia.c",
            --"contrib/SDL3/src/joystick/hidapi/SDL_hidapi_steam.c",
            --"contrib/SDL3/src/joystick/hidapi/SDL_hidapi_steam_hori.c",
            --"contrib/SDL3/src/joystick/hidapi/SDL_hidapi_steam_triton.c",
            --"contrib/SDL3/src/joystick/hidapi/SDL_hidapi_steamdeck.c",
            --"contrib/SDL3/src/joystick/hidapi/SDL_hidapi_switch.c",
            --"contrib/SDL3/src/joystick/hidapi/SDL_hidapi_switch2.c",
            --"contrib/SDL3/src/joystick/hidapi/SDL_hidapi_wii.c",
            --"contrib/SDL3/src/joystick/hidapi/SDL_hidapi_xbox360.c",
            --"contrib/SDL3/src/joystick/hidapi/SDL_hidapi_xbox360w.c",
            --"contrib/SDL3/src/joystick/hidapi/SDL_hidapi_xboxone.c",
            --"contrib/SDL3/src/joystick/hidapi/SDL_hidapi_zuiki.c",
            --"contrib/SDL3/src/joystick/hidapi/SDL_hidapijoystick.c",
            --"contrib/SDL3/src/joystick/hidapi/SDL_hidapijoystick_c.h",
            --"contrib/SDL3/src/joystick/hidapi/SDL_report_descriptor.c",
            --"contrib/SDL3/src/joystick/hidapi/SDL_report_descriptor.h",
            --"contrib/SDL3/src/joystick/hidapi/steam/controller_constants.h",
            --"contrib/SDL3/src/joystick/hidapi/steam/controller_structs.h",
            --"contrib/SDL3/src/joystick/sort_controllers.py",
            --"contrib/SDL3/src/joystick/usb_ids.h",
            --"contrib/SDL3/src/joystick/virtual/SDL_virtualjoystick.c",
            --"contrib/SDL3/src/joystick/virtual/SDL_virtualjoystick_c.h",
            --"contrib/SDL3/src/joystick/windows/SDL_dinputjoystick.c",
            --"contrib/SDL3/src/joystick/windows/SDL_dinputjoystick_c.h",
            --"contrib/SDL3/src/joystick/windows/SDL_rawinputjoystick.c",
            --"contrib/SDL3/src/joystick/windows/SDL_rawinputjoystick_c.h",
            --"contrib/SDL3/src/joystick/windows/SDL_windows_gaming_input.c",
            --"contrib/SDL3/src/joystick/windows/SDL_windowsjoystick.c",
            --"contrib/SDL3/src/joystick/windows/SDL_windowsjoystick_c.h",
            --"contrib/SDL3/src/joystick/windows/SDL_xinputjoystick.c",
            --"contrib/SDL3/src/joystick/windows/SDL_xinputjoystick_c.h",
            --"contrib/SDL3/src/libm/e_atan2.c",
            --"contrib/SDL3/src/libm/e_exp.c",
            --"contrib/SDL3/src/libm/e_fmod.c",
            --"contrib/SDL3/src/libm/e_log.c",
            --"contrib/SDL3/src/libm/e_log10.c",
            --"contrib/SDL3/src/libm/e_pow.c",
            --"contrib/SDL3/src/libm/e_rem_pio2.c",
            --"contrib/SDL3/src/libm/e_sqrt.c",
            --"contrib/SDL3/src/libm/k_cos.c",
            --"contrib/SDL3/src/libm/k_rem_pio2.c",
            --"contrib/SDL3/src/libm/k_sin.c",
            --"contrib/SDL3/src/libm/k_tan.c",
            --"contrib/SDL3/src/libm/math_libm.h",
            --"contrib/SDL3/src/libm/math_private.h",
            --"contrib/SDL3/src/libm/s_atan.c",
            --"contrib/SDL3/src/libm/s_copysign.c",
            --"contrib/SDL3/src/libm/s_cos.c",
            --"contrib/SDL3/src/libm/s_fabs.c",
            --"contrib/SDL3/src/libm/s_floor.c",
            --"contrib/SDL3/src/libm/s_isinf.c",
            --"contrib/SDL3/src/libm/s_isinff.c",
            --"contrib/SDL3/src/libm/s_isnan.c",
            --"contrib/SDL3/src/libm/s_isnanf.c",
            --"contrib/SDL3/src/libm/s_modf.c",
            --"contrib/SDL3/src/libm/s_scalbn.c",
            --"contrib/SDL3/src/libm/s_sin.c",
            --"contrib/SDL3/src/libm/s_tan.c",
            --"contrib/SDL3/src/loadso/dlopen/SDL_sysloadso.c",
            --"contrib/SDL3/src/loadso/dummy/SDL_sysloadso.c",
            --"contrib/SDL3/src/loadso/windows/SDL_sysloadso.c",
            --"contrib/SDL3/src/locale/SDL_locale.c",
            --"contrib/SDL3/src/locale/SDL_syslocale.h",
            --"contrib/SDL3/src/locale/dummy/SDL_syslocale.c",
            --"contrib/SDL3/src/locale/emscripten/SDL_syslocale.c",
            --"contrib/SDL3/src/locale/haiku/SDL_syslocale.cc",
            --"contrib/SDL3/src/locale/ngage/SDL_syslocale.cpp",
            --"contrib/SDL3/src/locale/windows/SDL_syslocale.c",
            --"contrib/SDL3/src/main/SDL_main_callbacks.c",
            --"contrib/SDL3/src/main/SDL_main_callbacks.h",
            --"contrib/SDL3/src/main/SDL_runapp.c",
            --"contrib/SDL3/src/main/emscripten/SDL_sysmain_callbacks.c",
            --"contrib/SDL3/src/main/emscripten/SDL_sysmain_runapp.c",
            --"contrib/SDL3/src/main/generic/SDL_sysmain_callbacks.c",
            --"contrib/SDL3/src/main/ngage/SDL_sysmain_callbacks.c",
            --"contrib/SDL3/src/main/ngage/SDL_sysmain_main.cpp",
            --"contrib/SDL3/src/main/ngage/SDL_sysmain_main.hpp",
            --"contrib/SDL3/src/main/windows/SDL_sysmain_runapp.c",
            --"contrib/SDL3/src/misc/SDL_libusb.c",
            --"contrib/SDL3/src/misc/SDL_libusb.h",
            --"contrib/SDL3/src/misc/SDL_sysurl.h",
            --"contrib/SDL3/src/misc/SDL_url.c",
            --"contrib/SDL3/src/misc/dummy/SDL_sysurl.c",
            --"contrib/SDL3/src/misc/emscripten/SDL_sysurl.c",
            --"contrib/SDL3/src/misc/haiku/SDL_sysurl.cc",
            --"contrib/SDL3/src/misc/riscos/SDL_sysurl.c",
            --"contrib/SDL3/src/misc/windows/SDL_sysurl.c",
            --"contrib/SDL3/src/power/SDL_power.c",
            --"contrib/SDL3/src/power/SDL_syspower.h",
            --"contrib/SDL3/src/power/emscripten/SDL_syspower.c",
            --"contrib/SDL3/src/power/haiku/SDL_syspower.c",
            --"contrib/SDL3/src/power/uikit/SDL_syspower.h",
            --"contrib/SDL3/src/power/uikit/SDL_syspower.m",
            --"contrib/SDL3/src/power/windows/SDL_syspower.c",
            --"contrib/SDL3/src/process/SDL_process.c",
            --"contrib/SDL3/src/process/SDL_sysprocess.h",
            --"contrib/SDL3/src/process/dummy/SDL_dummyprocess.c",
            --"contrib/SDL3/src/process/posix/SDL_posixprocess.c",
            --"contrib/SDL3/src/process/windows/SDL_windowsprocess.c",
            --"contrib/SDL3/src/render/SDL_d3dmath.h",
            --"contrib/SDL3/src/render/SDL_render.c",
            --"contrib/SDL3/src/render/SDL_render_debug_font.h",
            --"contrib/SDL3/src/render/SDL_render_unsupported.c",
            --"contrib/SDL3/src/render/SDL_sysrender.h",
            --"contrib/SDL3/src/render/SDL_yuv_sw.c",
            --"contrib/SDL3/src/render/SDL_yuv_sw_c.h",
            --"contrib/SDL3/src/render/direct3d/D3D9_PixelShader_Palette.h",
            --"contrib/SDL3/src/render/direct3d/D3D9_PixelShader_Palette_Linear.h",
            --"contrib/SDL3/src/render/direct3d/D3D9_PixelShader_Palette_Nearest.h",
            --"contrib/SDL3/src/render/direct3d/D3D9_PixelShader_YUV.h",
            --"contrib/SDL3/src/render/direct3d/SDL_render_d3d.c",
            --"contrib/SDL3/src/render/direct3d/SDL_shaders_d3d.c",
            --"contrib/SDL3/src/render/direct3d/SDL_shaders_d3d.h",
            --"contrib/SDL3/src/render/direct3d/compile_shaders.bat",
            --"contrib/SDL3/src/render/direct3d11/D3D11_PixelShader_Advanced.h",
            --"contrib/SDL3/src/render/direct3d11/D3D11_PixelShader_Colors.h",
            --"contrib/SDL3/src/render/direct3d11/D3D11_PixelShader_Textures.h",
            --"contrib/SDL3/src/render/direct3d11/D3D11_VertexShader.h",
            --"contrib/SDL3/src/render/direct3d11/SDL_render_d3d11.c",
            --"contrib/SDL3/src/render/direct3d11/SDL_shaders_d3d11.c",
            --"contrib/SDL3/src/render/direct3d11/SDL_shaders_d3d11.h",
            --"contrib/SDL3/src/render/direct3d11/compile_shaders.bat",
            --"contrib/SDL3/src/render/direct3d12/D3D12_PixelShader_Advanced.h",
            --"contrib/SDL3/src/render/direct3d12/D3D12_PixelShader_Colors.h",
            --"contrib/SDL3/src/render/direct3d12/D3D12_PixelShader_Textures.h",
            --"contrib/SDL3/src/render/direct3d12/D3D12_RootSig_Advanced.h",
            --"contrib/SDL3/src/render/direct3d12/D3D12_RootSig_Color.h",
            --"contrib/SDL3/src/render/direct3d12/D3D12_RootSig_Texture.h",
            --"contrib/SDL3/src/render/direct3d12/D3D12_VertexShader_Advanced.h",
            --"contrib/SDL3/src/render/direct3d12/D3D12_VertexShader_Color.h",
            --"contrib/SDL3/src/render/direct3d12/D3D12_VertexShader_Texture.h",
            --"contrib/SDL3/src/render/direct3d12/SDL_render_d3d12.c",
            --"contrib/SDL3/src/render/direct3d12/SDL_render_d3d12_xbox.cpp",
            --"contrib/SDL3/src/render/direct3d12/SDL_render_d3d12_xbox.h",
            --"contrib/SDL3/src/render/direct3d12/SDL_shaders_d3d12.c",
            --"contrib/SDL3/src/render/direct3d12/SDL_shaders_d3d12.h",
            --"contrib/SDL3/src/render/direct3d12/SDL_shaders_d3d12_xboxone.cpp",
            --"contrib/SDL3/src/render/direct3d12/SDL_shaders_d3d12_xboxseries.cpp",
            --"contrib/SDL3/src/render/direct3d12/compile_shaders.bat",
            --"contrib/SDL3/src/render/direct3d12/compile_shaders_xbox.bat",
            --"contrib/SDL3/src/render/gpu/SDL_gpu_util.h",
            --"contrib/SDL3/src/render/gpu/SDL_pipeline_gpu.c",
            --"contrib/SDL3/src/render/gpu/SDL_pipeline_gpu.h",
            --"contrib/SDL3/src/render/gpu/SDL_render_gpu.c",
            --"contrib/SDL3/src/render/gpu/SDL_shaders_gpu.c",
            --"contrib/SDL3/src/render/gpu/SDL_shaders_gpu.h",
            --"contrib/SDL3/src/render/gpu/shaders/.gitattributes",
            --"contrib/SDL3/src/render/gpu/shaders/.gitignore",
            --"contrib/SDL3/src/render/gpu/shaders/build-shaders.sh",
            --"contrib/SDL3/src/render/gpu/shaders/color.frag.dxil.h",
            --"contrib/SDL3/src/render/gpu/shaders/color.frag.msl.h",
            --"contrib/SDL3/src/render/gpu/shaders/color.frag.spv.h",
            --"contrib/SDL3/src/render/gpu/shaders/dxil.h",
            --"contrib/SDL3/src/render/gpu/shaders/linepoint.vert.dxil.h",
            --"contrib/SDL3/src/render/gpu/shaders/linepoint.vert.msl.h",
            --"contrib/SDL3/src/render/gpu/shaders/linepoint.vert.spv.h",
            --"contrib/SDL3/src/render/gpu/shaders/msl.h",
            --"contrib/SDL3/src/render/gpu/shaders/spir-v.h",
            --"contrib/SDL3/src/render/gpu/shaders/texture_advanced.frag.dxil.h",
            --"contrib/SDL3/src/render/gpu/shaders/texture_advanced.frag.msl.h",
            --"contrib/SDL3/src/render/gpu/shaders/texture_advanced.frag.spv.h",
            --"contrib/SDL3/src/render/gpu/shaders/texture_rgb.frag.dxil.h",
            --"contrib/SDL3/src/render/gpu/shaders/texture_rgb.frag.msl.h",
            --"contrib/SDL3/src/render/gpu/shaders/texture_rgb.frag.spv.h",
            --"contrib/SDL3/src/render/gpu/shaders/texture_rgba.frag.dxil.h",
            --"contrib/SDL3/src/render/gpu/shaders/texture_rgba.frag.msl.h",
            --"contrib/SDL3/src/render/gpu/shaders/texture_rgba.frag.spv.h",
            --"contrib/SDL3/src/render/gpu/shaders/tri_color.vert.dxil.h",
            --"contrib/SDL3/src/render/gpu/shaders/tri_color.vert.msl.h",
            --"contrib/SDL3/src/render/gpu/shaders/tri_color.vert.spv.h",
            --"contrib/SDL3/src/render/gpu/shaders/tri_texture.vert.dxil.h",
            --"contrib/SDL3/src/render/gpu/shaders/tri_texture.vert.msl.h",
            --"contrib/SDL3/src/render/gpu/shaders/tri_texture.vert.spv.h",
            --"contrib/SDL3/src/render/ngage/SDL_render_ngage.c",
            --"contrib/SDL3/src/render/ngage/SDL_render_ngage.cpp",
            --"contrib/SDL3/src/render/ngage/SDL_render_ngage_c.h",
            --"contrib/SDL3/src/render/ngage/SDL_render_ngage_c.hpp",
            --"contrib/SDL3/src/render/ngage/SDL_render_ops.cpp",
            --"contrib/SDL3/src/render/ngage/SDL_render_ops.hpp",
            --"contrib/SDL3/src/render/opengl/SDL_glfuncs.h",
            --"contrib/SDL3/src/render/opengl/SDL_render_gl.c",
            --"contrib/SDL3/src/render/opengl/SDL_shaders_gl.c",
            --"contrib/SDL3/src/render/opengl/SDL_shaders_gl.h",
            --"contrib/SDL3/src/render/opengles2/SDL_gles2funcs.h",
            --"contrib/SDL3/src/render/opengles2/SDL_render_gles2.c",
            --"contrib/SDL3/src/render/opengles2/SDL_shaders_gles2.c",
            --"contrib/SDL3/src/render/opengles2/SDL_shaders_gles2.h",
            --"contrib/SDL3/src/render/software/SDL_blendfillrect.c",
            --"contrib/SDL3/src/render/software/SDL_blendfillrect.h",
            --"contrib/SDL3/src/render/software/SDL_blendline.c",
            --"contrib/SDL3/src/render/software/SDL_blendline.h",
            --"contrib/SDL3/src/render/software/SDL_blendpoint.c",
            --"contrib/SDL3/src/render/software/SDL_blendpoint.h",
            --"contrib/SDL3/src/render/software/SDL_draw.h",
            --"contrib/SDL3/src/render/software/SDL_drawline.c",
            --"contrib/SDL3/src/render/software/SDL_drawline.h",
            --"contrib/SDL3/src/render/software/SDL_drawpoint.c",
            --"contrib/SDL3/src/render/software/SDL_drawpoint.h",
            --"contrib/SDL3/src/render/software/SDL_render_sw.c",
            --"contrib/SDL3/src/render/software/SDL_render_sw_c.h",
            --"contrib/SDL3/src/render/software/SDL_triangle.c",
            --"contrib/SDL3/src/render/software/SDL_triangle.h",
            --"contrib/SDL3/src/render/vitagxm/SDL_render_vita_gxm.c",
            --"contrib/SDL3/src/render/vitagxm/SDL_render_vita_gxm_memory.c",
            --"contrib/SDL3/src/render/vitagxm/SDL_render_vita_gxm_memory.h",
            --"contrib/SDL3/src/render/vitagxm/SDL_render_vita_gxm_shaders.h",
            --"contrib/SDL3/src/render/vitagxm/SDL_render_vita_gxm_tools.c",
            --"contrib/SDL3/src/render/vitagxm/SDL_render_vita_gxm_tools.h",
            --"contrib/SDL3/src/render/vitagxm/SDL_render_vita_gxm_types.h",
            --"contrib/SDL3/src/render/vitagxm/shader_src/clear_f.cg",
            --"contrib/SDL3/src/render/vitagxm/shader_src/clear_v.cg",
            --"contrib/SDL3/src/render/vitagxm/shader_src/color_f.cg",
            --"contrib/SDL3/src/render/vitagxm/shader_src/color_v.cg",
            --"contrib/SDL3/src/render/vitagxm/shader_src/texture_f.cg",
            --"contrib/SDL3/src/render/vitagxm/shader_src/texture_v.cg",
            --"contrib/SDL3/src/render/vulkan/SDL_render_vulkan.c",
            --"contrib/SDL3/src/render/vulkan/SDL_shaders_vulkan.c",
            --"contrib/SDL3/src/render/vulkan/SDL_shaders_vulkan.h",
            --"contrib/SDL3/src/render/vulkan/VULKAN_PixelShader_Advanced.h",
            --"contrib/SDL3/src/render/vulkan/VULKAN_PixelShader_Colors.h",
            --"contrib/SDL3/src/render/vulkan/VULKAN_PixelShader_Textures.h",
            --"contrib/SDL3/src/render/vulkan/VULKAN_VertexShader.h",
            --"contrib/SDL3/src/render/vulkan/compile_shaders.bat",
            --"contrib/SDL3/src/sensor/SDL_sensor.c",
            --"contrib/SDL3/src/sensor/SDL_sensor_c.h",
            --"contrib/SDL3/src/sensor/SDL_syssensor.h",
            --"contrib/SDL3/src/sensor/coremotion/SDL_coremotionsensor.h",
            --"contrib/SDL3/src/sensor/coremotion/SDL_coremotionsensor.m",
            --"contrib/SDL3/src/sensor/dummy/SDL_dummysensor.c",
            --"contrib/SDL3/src/sensor/dummy/SDL_dummysensor.h",
            --"contrib/SDL3/src/sensor/emscripten/SDL_emscriptensensor.c",
            --"contrib/SDL3/src/sensor/emscripten/SDL_emscriptensensor.h",
            --"contrib/SDL3/src/sensor/windows/SDL_windowssensor.c",
            --"contrib/SDL3/src/sensor/windows/SDL_windowssensor.h",
            --"contrib/SDL3/src/stdlib/SDL_casefolding.h",
            --"contrib/SDL3/src/stdlib/SDL_crc16.c",
            --"contrib/SDL3/src/stdlib/SDL_crc32.c",
            --"contrib/SDL3/src/stdlib/SDL_getenv.c",
            --"contrib/SDL3/src/stdlib/SDL_getenv_c.h",
            --"contrib/SDL3/src/stdlib/SDL_iconv.c",
            --"contrib/SDL3/src/stdlib/SDL_malloc.c",
            --"contrib/SDL3/src/stdlib/SDL_memcpy.c",
            --"contrib/SDL3/src/stdlib/SDL_memmove.c",
            --"contrib/SDL3/src/stdlib/SDL_memset.c",
            --"contrib/SDL3/src/stdlib/SDL_mslibc.c",
            --"contrib/SDL3/src/stdlib/SDL_mslibc_arm64.masm",
            --"contrib/SDL3/src/stdlib/SDL_mslibc_x64.masm",
            --"contrib/SDL3/src/stdlib/SDL_murmur3.c",
            --"contrib/SDL3/src/stdlib/SDL_qsort.c",
            --"contrib/SDL3/src/stdlib/SDL_random.c",
            --"contrib/SDL3/src/stdlib/SDL_stdlib.c",
            --"contrib/SDL3/src/stdlib/SDL_string.c",
            --"contrib/SDL3/src/stdlib/SDL_strtokr.c",
            --"contrib/SDL3/src/stdlib/SDL_sysstdlib.h",
            --"contrib/SDL3/src/stdlib/SDL_vacopy.h",
            --"contrib/SDL3/src/storage/SDL_storage.c",
            --"contrib/SDL3/src/storage/SDL_sysstorage.h",
            --"contrib/SDL3/src/storage/generic/SDL_genericstorage.c",
            --"contrib/SDL3/src/storage/steam/SDL_steamstorage.c",
            --"contrib/SDL3/src/storage/steam/SDL_steamstorage_proc.h",
            --"contrib/SDL3/src/test/SDL_test_assert.c",
            --"contrib/SDL3/src/test/SDL_test_common.c",
            --"contrib/SDL3/src/test/SDL_test_compare.c",
            --"contrib/SDL3/src/test/SDL_test_crc32.c",
            --"contrib/SDL3/src/test/SDL_test_font.c",
            --"contrib/SDL3/src/test/SDL_test_fuzzer.c",
            --"contrib/SDL3/src/test/SDL_test_harness.c",
            --"contrib/SDL3/src/test/SDL_test_internal.h",
            --"contrib/SDL3/src/test/SDL_test_log.c",
            --"contrib/SDL3/src/test/SDL_test_md5.c",
            --"contrib/SDL3/src/test/SDL_test_memory.c",
            --"contrib/SDL3/src/thread/SDL_systhread.h",
            --"contrib/SDL3/src/thread/SDL_thread.c",
            --"contrib/SDL3/src/thread/SDL_thread_c.h",
            --"contrib/SDL3/src/thread/generic/SDL_syscond.c",
            --"contrib/SDL3/src/thread/generic/SDL_syscond_c.h",
            --"contrib/SDL3/src/thread/generic/SDL_sysmutex.c",
            --"contrib/SDL3/src/thread/generic/SDL_sysmutex_c.h",
            --"contrib/SDL3/src/thread/generic/SDL_sysrwlock.c",
            --"contrib/SDL3/src/thread/generic/SDL_sysrwlock_c.h",
            --"contrib/SDL3/src/thread/generic/SDL_syssem.c",
            --"contrib/SDL3/src/thread/generic/SDL_systhread.c",
            --"contrib/SDL3/src/thread/generic/SDL_systhread_c.h",
            --"contrib/SDL3/src/thread/generic/SDL_systls.c",
            --"contrib/SDL3/src/thread/pthread/SDL_syscond.c",
            --"contrib/SDL3/src/thread/pthread/SDL_sysmutex.c",
            --"contrib/SDL3/src/thread/pthread/SDL_sysmutex_c.h",
            --"contrib/SDL3/src/thread/pthread/SDL_sysrwlock.c",
            --"contrib/SDL3/src/thread/pthread/SDL_syssem.c",
            --"contrib/SDL3/src/thread/pthread/SDL_systhread.c",
            --"contrib/SDL3/src/thread/pthread/SDL_systhread_c.h",
            --"contrib/SDL3/src/thread/pthread/SDL_systls.c",
            --"contrib/SDL3/src/thread/windows/SDL_syscond_cv.c",
            --"contrib/SDL3/src/thread/windows/SDL_sysmutex.c",
            --"contrib/SDL3/src/thread/windows/SDL_sysmutex_c.h",
            --"contrib/SDL3/src/thread/windows/SDL_sysrwlock_srw.c",
            --"contrib/SDL3/src/thread/windows/SDL_syssem.c",
            --"contrib/SDL3/src/thread/windows/SDL_systhread.c",
            --"contrib/SDL3/src/thread/windows/SDL_systhread_c.h",
            --"contrib/SDL3/src/thread/windows/SDL_systls.c",
            --"contrib/SDL3/src/time/SDL_time.c",
            --"contrib/SDL3/src/time/SDL_time_c.h",
            --"contrib/SDL3/src/time/ngage/SDL_systime.cpp",
            --"contrib/SDL3/src/time/windows/SDL_systime.c",
            --"contrib/SDL3/src/timer/SDL_timer.c",
            --"contrib/SDL3/src/timer/SDL_timer_c.h",
            --"contrib/SDL3/src/timer/haiku/SDL_systimer.c",
            --"contrib/SDL3/src/timer/ngage/SDL_systimer.cpp",
            --"contrib/SDL3/src/timer/windows/SDL_systimer.c",
            --"contrib/SDL3/src/tray/SDL_tray_utils.c",
            --"contrib/SDL3/src/tray/SDL_tray_utils.h",
            --"contrib/SDL3/src/tray/dummy/SDL_tray.c",
            --"contrib/SDL3/src/tray/windows/SDL_tray.c",
            --"contrib/SDL3/src/video/SDL_RLEaccel.c",
            --"contrib/SDL3/src/video/SDL_RLEaccel_c.h",
            --"contrib/SDL3/src/video/SDL_blit.c",
            --"contrib/SDL3/src/video/SDL_blit.h",
            --"contrib/SDL3/src/video/SDL_blit_0.c",
            --"contrib/SDL3/src/video/SDL_blit_1.c",
            --"contrib/SDL3/src/video/SDL_blit_A.c",
            --"contrib/SDL3/src/video/SDL_blit_N.c",
            --"contrib/SDL3/src/video/SDL_blit_auto.c",
            --"contrib/SDL3/src/video/SDL_blit_auto.h",
            --"contrib/SDL3/src/video/SDL_blit_copy.c",
            --"contrib/SDL3/src/video/SDL_blit_copy.h",
            --"contrib/SDL3/src/video/SDL_blit_slow.c",
            --"contrib/SDL3/src/video/SDL_blit_slow.h",
            --"contrib/SDL3/src/video/SDL_bmp.c",
            --"contrib/SDL3/src/video/SDL_clipboard.c",
            --"contrib/SDL3/src/video/SDL_clipboard_c.h",
            --"contrib/SDL3/src/video/SDL_egl.c",
            --"contrib/SDL3/src/video/SDL_egl_c.h",
            --"contrib/SDL3/src/video/SDL_fillrect.c",
            --"contrib/SDL3/src/video/SDL_pixels.c",
            --"contrib/SDL3/src/video/SDL_pixels_c.h",
            --"contrib/SDL3/src/video/SDL_rect.c",
            --"contrib/SDL3/src/video/SDL_rect_c.h",
            --"contrib/SDL3/src/video/SDL_rect_impl.h",
            --"contrib/SDL3/src/video/SDL_rotate.c",
            --"contrib/SDL3/src/video/SDL_rotate.h",
            --"contrib/SDL3/src/video/SDL_stb.c",
            --"contrib/SDL3/src/video/SDL_stb_c.h",
            --"contrib/SDL3/src/video/SDL_stretch.c",
            --"contrib/SDL3/src/video/SDL_surface.c",
            --"contrib/SDL3/src/video/SDL_surface_c.h",
            --"contrib/SDL3/src/video/SDL_sysvideo.h",
            --"contrib/SDL3/src/video/SDL_video.c",
            --"contrib/SDL3/src/video/SDL_video_c.h",
            --"contrib/SDL3/src/video/SDL_video_unsupported.c",
            --"contrib/SDL3/src/video/SDL_video_unsupported.h",
            --"contrib/SDL3/src/video/SDL_vulkan_internal.h",
            --"contrib/SDL3/src/video/SDL_vulkan_utils.c",
            --"contrib/SDL3/src/video/SDL_yuv.c",
            --"contrib/SDL3/src/video/SDL_yuv_c.h",
            --"contrib/SDL3/src/video/directx/SDL_d3d12.h",
            --"contrib/SDL3/src/video/directx/d3d12.h",
            --"contrib/SDL3/src/video/directx/d3d12sdklayers.h",
            --"contrib/SDL3/src/video/dummy/SDL_nullevents.c",
            --"contrib/SDL3/src/video/dummy/SDL_nullevents_c.h",
            --"contrib/SDL3/src/video/dummy/SDL_nullframebuffer.c",
            --"contrib/SDL3/src/video/dummy/SDL_nullframebuffer_c.h",
            --"contrib/SDL3/src/video/dummy/SDL_nullvideo.c",
            --"contrib/SDL3/src/video/dummy/SDL_nullvideo.h",
            --"contrib/SDL3/src/video/emscripten/SDL_emscriptenevents.c",
            --"contrib/SDL3/src/video/emscripten/SDL_emscriptenevents.h",
            --"contrib/SDL3/src/video/emscripten/SDL_emscriptenframebuffer.c",
            --"contrib/SDL3/src/video/emscripten/SDL_emscriptenframebuffer.h",
            --"contrib/SDL3/src/video/emscripten/SDL_emscriptenmouse.c",
            --"contrib/SDL3/src/video/emscripten/SDL_emscriptenmouse.h",
            --"contrib/SDL3/src/video/emscripten/SDL_emscriptenopengles.c",
            --"contrib/SDL3/src/video/emscripten/SDL_emscriptenopengles.h",
            --"contrib/SDL3/src/video/emscripten/SDL_emscriptenvideo.c",
            --"contrib/SDL3/src/video/emscripten/SDL_emscriptenvideo.h",
            --"contrib/SDL3/src/video/haiku/SDL_BWin.h",
            --"contrib/SDL3/src/video/haiku/SDL_bclipboard.cc",
            --"contrib/SDL3/src/video/haiku/SDL_bclipboard.h",
            --"contrib/SDL3/src/video/haiku/SDL_bevents.cc",
            --"contrib/SDL3/src/video/haiku/SDL_bevents.h",
            --"contrib/SDL3/src/video/haiku/SDL_bframebuffer.cc",
            --"contrib/SDL3/src/video/haiku/SDL_bframebuffer.h",
            --"contrib/SDL3/src/video/haiku/SDL_bkeyboard.cc",
            --"contrib/SDL3/src/video/haiku/SDL_bkeyboard.h",
            --"contrib/SDL3/src/video/haiku/SDL_bmessagebox.cc",
            --"contrib/SDL3/src/video/haiku/SDL_bmessagebox.h",
            --"contrib/SDL3/src/video/haiku/SDL_bmodes.cc",
            --"contrib/SDL3/src/video/haiku/SDL_bmodes.h",
            --"contrib/SDL3/src/video/haiku/SDL_bopengl.cc",
            --"contrib/SDL3/src/video/haiku/SDL_bopengl.h",
            --"contrib/SDL3/src/video/haiku/SDL_bvideo.cc",
            --"contrib/SDL3/src/video/haiku/SDL_bvideo.h",
            --"contrib/SDL3/src/video/haiku/SDL_bwindow.cc",
            --"contrib/SDL3/src/video/haiku/SDL_bwindow.h",
            --"contrib/SDL3/src/video/khronos/EGL/egl.h",
            --"contrib/SDL3/src/video/khronos/EGL/eglext.h",
            --"contrib/SDL3/src/video/khronos/EGL/eglplatform.h",
            --"contrib/SDL3/src/video/khronos/GLES2/gl2.h",
            --"contrib/SDL3/src/video/khronos/GLES2/gl2ext.h",
            --"contrib/SDL3/src/video/khronos/GLES2/gl2platform.h",
            --"contrib/SDL3/src/video/khronos/GLES3/gl3.h",
            --"contrib/SDL3/src/video/khronos/GLES3/gl31.h",
            --"contrib/SDL3/src/video/khronos/GLES3/gl32.h",
            --"contrib/SDL3/src/video/khronos/GLES3/gl3platform.h",
            --"contrib/SDL3/src/video/khronos/KHR/khrplatform.h",
            --"contrib/SDL3/src/video/khronos/vk_video/vulkan_video_codec_av1std.h",
            --"contrib/SDL3/src/video/khronos/vk_video/vulkan_video_codec_av1std_decode.h",
            --"contrib/SDL3/src/video/khronos/vk_video/vulkan_video_codec_h264std.h",
            --"contrib/SDL3/src/video/khronos/vk_video/vulkan_video_codec_h264std_decode.h",
            --"contrib/SDL3/src/video/khronos/vk_video/vulkan_video_codec_h264std_encode.h",
            --"contrib/SDL3/src/video/khronos/vk_video/vulkan_video_codec_h265std.h",
            --"contrib/SDL3/src/video/khronos/vk_video/vulkan_video_codec_h265std_decode.h",
            --"contrib/SDL3/src/video/khronos/vk_video/vulkan_video_codec_h265std_encode.h",
            --"contrib/SDL3/src/video/khronos/vk_video/vulkan_video_codecs_common.h",
            --"contrib/SDL3/src/video/khronos/vulkan/vk_icd.h",
            --"contrib/SDL3/src/video/khronos/vulkan/vk_layer.h",
            --"contrib/SDL3/src/video/khronos/vulkan/vk_platform.h",
            --"contrib/SDL3/src/video/khronos/vulkan/vulkan.h",
            --"contrib/SDL3/src/video/khronos/vulkan/vulkan_beta.h",
            --"contrib/SDL3/src/video/khronos/vulkan/vulkan_core.h",
            --"contrib/SDL3/src/video/khronos/vulkan/vulkan_directfb.h",
            --"contrib/SDL3/src/video/khronos/vulkan/vulkan_fuchsia.h",
            --"contrib/SDL3/src/video/khronos/vulkan/vulkan_ggp.h",
            --"contrib/SDL3/src/video/khronos/vulkan/vulkan_screen.h",
            --"contrib/SDL3/src/video/khronos/vulkan/vulkan_vi.h",
            --"contrib/SDL3/src/video/khronos/vulkan/vulkan_wayland.h",
            --"contrib/SDL3/src/video/khronos/vulkan/vulkan_win32.h",
            --"contrib/SDL3/src/video/khronos/vulkan/vulkan_xcb.h",
            --"contrib/SDL3/src/video/khronos/vulkan/vulkan_xlib.h",
            --"contrib/SDL3/src/video/khronos/vulkan/vulkan_xlib_xrandr.h",
            --"contrib/SDL3/src/video/kmsdrm/SDL_kmsdrmdyn.c",
            --"contrib/SDL3/src/video/kmsdrm/SDL_kmsdrmdyn.h",
            --"contrib/SDL3/src/video/kmsdrm/SDL_kmsdrmevents.c",
            --"contrib/SDL3/src/video/kmsdrm/SDL_kmsdrmevents.h",
            --"contrib/SDL3/src/video/kmsdrm/SDL_kmsdrmmouse.c",
            --"contrib/SDL3/src/video/kmsdrm/SDL_kmsdrmmouse.h",
            --"contrib/SDL3/src/video/kmsdrm/SDL_kmsdrmopengles.c",
            --"contrib/SDL3/src/video/kmsdrm/SDL_kmsdrmopengles.h",
            --"contrib/SDL3/src/video/kmsdrm/SDL_kmsdrmsym.h",
            --"contrib/SDL3/src/video/kmsdrm/SDL_kmsdrmvideo.c",
            --"contrib/SDL3/src/video/kmsdrm/SDL_kmsdrmvideo.h",
            --"contrib/SDL3/src/video/kmsdrm/SDL_kmsdrmvulkan.c",
            --"contrib/SDL3/src/video/kmsdrm/SDL_kmsdrmvulkan.h",
            --"contrib/SDL3/src/video/miniz.h",
            --"contrib/SDL3/src/video/ngage/SDL_ngagevideo.c",
            --"contrib/SDL3/src/video/ngage/SDL_ngagevideo.h",
            --"contrib/SDL3/src/video/offscreen/SDL_offscreenevents.c",
            --"contrib/SDL3/src/video/offscreen/SDL_offscreenevents_c.h",
            --"contrib/SDL3/src/video/offscreen/SDL_offscreenframebuffer.c",
            --"contrib/SDL3/src/video/offscreen/SDL_offscreenframebuffer_c.h",
            --"contrib/SDL3/src/video/offscreen/SDL_offscreenopengles.c",
            --"contrib/SDL3/src/video/offscreen/SDL_offscreenopengles.h",
            --"contrib/SDL3/src/video/offscreen/SDL_offscreenvideo.c",
            --"contrib/SDL3/src/video/offscreen/SDL_offscreenvideo.h",
            --"contrib/SDL3/src/video/offscreen/SDL_offscreenvulkan.c",
            --"contrib/SDL3/src/video/offscreen/SDL_offscreenvulkan.h",
            --"contrib/SDL3/src/video/offscreen/SDL_offscreenwindow.c",
            --"contrib/SDL3/src/video/offscreen/SDL_offscreenwindow.h",
            --"contrib/SDL3/src/video/qnx/SDL_qnx.h",
            --"contrib/SDL3/src/video/qnx/SDL_qnxgl.c",
            --"contrib/SDL3/src/video/qnx/SDL_qnxkeyboard.c",
            --"contrib/SDL3/src/video/qnx/SDL_qnxvideo.c",
            --"contrib/SDL3/src/video/riscos/SDL_riscosdefs.h",
            --"contrib/SDL3/src/video/riscos/SDL_riscosevents.c",
            --"contrib/SDL3/src/video/riscos/SDL_riscosevents_c.h",
            --"contrib/SDL3/src/video/riscos/SDL_riscosframebuffer.c",
            --"contrib/SDL3/src/video/riscos/SDL_riscosframebuffer_c.h",
            --"contrib/SDL3/src/video/riscos/SDL_riscosmessagebox.c",
            --"contrib/SDL3/src/video/riscos/SDL_riscosmessagebox.h",
            --"contrib/SDL3/src/video/riscos/SDL_riscosmodes.c",
            --"contrib/SDL3/src/video/riscos/SDL_riscosmodes.h",
            --"contrib/SDL3/src/video/riscos/SDL_riscosmouse.c",
            --"contrib/SDL3/src/video/riscos/SDL_riscosmouse.h",
            --"contrib/SDL3/src/video/riscos/SDL_riscosvideo.c",
            --"contrib/SDL3/src/video/riscos/SDL_riscosvideo.h",
            --"contrib/SDL3/src/video/riscos/SDL_riscoswindow.c",
            --"contrib/SDL3/src/video/riscos/SDL_riscoswindow.h",
            --"contrib/SDL3/src/video/riscos/scancodes_riscos.h",
            --"contrib/SDL3/src/video/sdlgenblit.pl",
            --"contrib/SDL3/src/video/stb_image.h",
            --"contrib/SDL3/src/video/uikit/SDL_uikitappdelegate.h",
            --"contrib/SDL3/src/video/uikit/SDL_uikitappdelegate.m",
            --"contrib/SDL3/src/video/uikit/SDL_uikitclipboard.h",
            --"contrib/SDL3/src/video/uikit/SDL_uikitclipboard.m",
            --"contrib/SDL3/src/video/uikit/SDL_uikitevents.h",
            --"contrib/SDL3/src/video/uikit/SDL_uikitevents.m",
            --"contrib/SDL3/src/video/uikit/SDL_uikitmessagebox.h",
            --"contrib/SDL3/src/video/uikit/SDL_uikitmessagebox.m",
            --"contrib/SDL3/src/video/uikit/SDL_uikitmodes.h",
            --"contrib/SDL3/src/video/uikit/SDL_uikitmodes.m",
            --"contrib/SDL3/src/video/uikit/SDL_uikitopengles.h",
            --"contrib/SDL3/src/video/uikit/SDL_uikitopengles.m",
            --"contrib/SDL3/src/video/uikit/SDL_uikitopenglview.h",
            --"contrib/SDL3/src/video/uikit/SDL_uikitopenglview.m",
            --"contrib/SDL3/src/video/uikit/SDL_uikitpen.h",
            --"contrib/SDL3/src/video/uikit/SDL_uikitpen.m",
            --"contrib/SDL3/src/video/uikit/SDL_uikitvideo.h",
            --"contrib/SDL3/src/video/uikit/SDL_uikitvideo.m",
            --"contrib/SDL3/src/video/uikit/SDL_uikitview.h",
            --"contrib/SDL3/src/video/uikit/SDL_uikitview.m",
            --"contrib/SDL3/src/video/uikit/SDL_uikitviewcontroller.h",
            --"contrib/SDL3/src/video/uikit/SDL_uikitviewcontroller.m",
            --"contrib/SDL3/src/video/uikit/SDL_uikitvulkan.h",
            --"contrib/SDL3/src/video/uikit/SDL_uikitvulkan.m",
            --"contrib/SDL3/src/video/uikit/SDL_uikitwindow.h",
            --"contrib/SDL3/src/video/uikit/SDL_uikitwindow.m",
            --"contrib/SDL3/src/video/vivante/SDL_vivanteopengles.c",
            --"contrib/SDL3/src/video/vivante/SDL_vivanteopengles.h",
            --"contrib/SDL3/src/video/vivante/SDL_vivanteplatform.c",
            --"contrib/SDL3/src/video/vivante/SDL_vivanteplatform.h",
            --"contrib/SDL3/src/video/vivante/SDL_vivantevideo.c",
            --"contrib/SDL3/src/video/vivante/SDL_vivantevideo.h",
            --"contrib/SDL3/src/video/vivante/SDL_vivantevulkan.c",
            --"contrib/SDL3/src/video/vivante/SDL_vivantevulkan.h",
            --"contrib/SDL3/src/video/windows/SDL_msctf.h",
            --"contrib/SDL3/src/video/windows/SDL_windowsclipboard.c",
            --"contrib/SDL3/src/video/windows/SDL_windowsclipboard.h",
            --"contrib/SDL3/src/video/windows/SDL_windowsevents.c",
            --"contrib/SDL3/src/video/windows/SDL_windowsevents.h",
            --"contrib/SDL3/src/video/windows/SDL_windowsframebuffer.c",
            --"contrib/SDL3/src/video/windows/SDL_windowsframebuffer.h",
            --"contrib/SDL3/src/video/windows/SDL_windowsgameinput.cpp",
            --"contrib/SDL3/src/video/windows/SDL_windowsgameinput.h",
            --"contrib/SDL3/src/video/windows/SDL_windowskeyboard.c",
            --"contrib/SDL3/src/video/windows/SDL_windowskeyboard.h",
            --"contrib/SDL3/src/video/windows/SDL_windowsmessagebox.c",
            --"contrib/SDL3/src/video/windows/SDL_windowsmessagebox.h",
            --"contrib/SDL3/src/video/windows/SDL_windowsmodes.c",
            --"contrib/SDL3/src/video/windows/SDL_windowsmodes.h",
            --"contrib/SDL3/src/video/windows/SDL_windowsmouse.c",
            --"contrib/SDL3/src/video/windows/SDL_windowsmouse.h",
            --"contrib/SDL3/src/video/windows/SDL_windowsopengl.c",
            --"contrib/SDL3/src/video/windows/SDL_windowsopengl.h",
            --"contrib/SDL3/src/video/windows/SDL_windowsopengles.c",
            --"contrib/SDL3/src/video/windows/SDL_windowsopengles.h",
            --"contrib/SDL3/src/video/windows/SDL_windowsrawinput.c",
            --"contrib/SDL3/src/video/windows/SDL_windowsrawinput.h",
            --"contrib/SDL3/src/video/windows/SDL_windowsshape.c",
            --"contrib/SDL3/src/video/windows/SDL_windowsshape.h",
            --"contrib/SDL3/src/video/windows/SDL_windowsvideo.c",
            --"contrib/SDL3/src/video/windows/SDL_windowsvideo.h",
            --"contrib/SDL3/src/video/windows/SDL_windowsvulkan.c",
            --"contrib/SDL3/src/video/windows/SDL_windowsvulkan.h",
            --"contrib/SDL3/src/video/windows/SDL_windowswindow.c",
            --"contrib/SDL3/src/video/windows/SDL_windowswindow.h",
            --"contrib/SDL3/src/video/windows/wmmsg.h",
            --"contrib/SDL3/src/video/yuv2rgb/LICENSE",
            --"contrib/SDL3/src/video/yuv2rgb/README.md",
            --"contrib/SDL3/src/video/yuv2rgb/yuv_rgb.h",
            --"contrib/SDL3/src/video/yuv2rgb/yuv_rgb_common.h",
            --"contrib/SDL3/src/video/yuv2rgb/yuv_rgb_internal.h",
            --"contrib/SDL3/src/video/yuv2rgb/yuv_rgb_lsx.c",
            --"contrib/SDL3/src/video/yuv2rgb/yuv_rgb_lsx.h",
            --"contrib/SDL3/src/video/yuv2rgb/yuv_rgb_lsx_func.h",
            --"contrib/SDL3/src/video/yuv2rgb/yuv_rgb_sse.c",
            --"contrib/SDL3/src/video/yuv2rgb/yuv_rgb_sse.h",
            --"contrib/SDL3/src/video/yuv2rgb/yuv_rgb_sse_func.h",
            --"contrib/SDL3/src/video/yuv2rgb/yuv_rgb_std.c",
            --"contrib/SDL3/src/video/yuv2rgb/yuv_rgb_std.h",
            --"contrib/SDL3/src/video/yuv2rgb/yuv_rgb_std_func.h",

            --"contrib/SDL3/src/*.c",
            --"contrib/SDL3/src/atomic/*.c",
            --"contrib/SDL3/src/audio/*.c",
            --"contrib/SDL3/src/camera/*.c",
            --"contrib/SDL3/src/core/*.c",
            --"contrib/SDL3/src/core/windows/*.c",
            --"contrib/SDL3/src/cpuinfo/*.c",
            --"contrib/SDL3/src/dialog/*.c",
            --"contrib/SDL3/src/dynapi/*.c",
            --"contrib/SDL3/src/events/*.c",
            --"contrib/SDL3/src/filesystem/*.c",
            --"contrib/SDL3/src/gpu/*.c",

            --"contrib/SDL3/src/atomic/*.c",
            --"contrib/SDL3/src/core/windows/*.c",
            --"contrib/SDL3/src/cpuinfo/*.c",
            --"contrib/SDL3/src/events/*.c",
            --"contrib/SDL3/src/filesystem/windows/*.c",
            --"contrib/SDL3/src/haptic/windows/*.c",
            --"contrib/SDL3/src/joystick/windows/*.c",
            --"contrib/SDL3/src/joystick/hidapi/*.c",
            --"contrib/SDL3/src/loadso/windows/*.c",
            --"contrib/SDL3/src/locale/windows/*.c",
            --"contrib/SDL3/src/misc/windows/*.c",
            --"contrib/SDL3/src/power/windows/*.c",
            --"contrib/SDL3/src/process/windows/*.c",
            --"contrib/SDL3/src/render/*.c",
            --"contrib/SDL3/src/render/direct3d11/*.c",
            --"contrib/SDL3/src/render/direct3d12/*.c",
            --"contrib/SDL3/src/render/direct3d/*.c",
            --"contrib/SDL3/src/render/software/*.c",
            --"contrib/SDL3/src/thread/windows/*.c",
            --"contrib/SDL3/src/timer/windows/*.c",
            --"contrib/SDL3/src/video/windows/*.c",
        }

        defines {
            "SDL_STATIC_LIB",
            "SDL_BUILDING_LIBRARY",

            "SDL_VIDEO_DRIVER_WINDOWS",
            "SDL_FILESYSTEM_WINDOWS",
            "SDL_LOADSO_WINDOWS",
            "SDL_THREAD_WINDOWS",
            "SDL_TIMER_WINDOWS",
            "SDL_POWER_WINDOWS",
            "SDL_JOYSTICK_WINDOWS",
            "SDL_GAMEINPUT_WINDOWS",
            "SDL_HAPTIC_WINDOWS",

            --"SDL_AUDIO_DRIVER_DUMMY",
            --"SDL_VIDEO_DRIVER_WINDOWS",
            --"SDL_FILESYSTEM_WINDOWS",
            --"SDL_LOADSO_WINDOWS",
            --"SDL_THREAD_WINDOWS",
            --"SDL_TIMER_WINDOWS",
            --"SDL_POWER_WINDOWS",
            --"SDL_JOYSTICK_WINDOWS",
            --"SDL_HAPTIC_WINDOWS",

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
