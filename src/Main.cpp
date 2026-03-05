// Dear ImGui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#include "imgui.h"
#include "ImGui/backends/imgui_impl_glfw.h"
#include "ImGui/backends/imgui_impl_opengl3.h"
#include <stdio.h>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif

#include "Tracy.hpp"
#include "cmdline.h"

#include "WinInterop.h"
#include "WinInterop_File.h"
#include "Math.h"
#include "Threading.h"
#include "Themes.h"
#include "Settings.h"
#include "LoadJson.h"
#include "ImguiHelper.h"
#include "resource.h"
#include "Citect.h"
#include "Rendering.h"
#include "Networking.h"

#include <stdio.h>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

// This example can also compile and run with Emscripten! See 'Makefile.emscripten' for details.
#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

int Main(int argc, char** argv)
{
    DebugPrint("====================");
    DebugPrint("     Quool Tool     ");
    DebugPrint("====================");
#if 0
    //ERROR: Threading will not work correctly unless InitOS() is called before
    //argc will always be 1 in console mode
    if (**argv != 0 && (argc == -1 || argc > 1))
    {
        cmdline::parser p;
        const char* backup_name = "backup-path";
        const char* citect_project_path_name = "citect-project-path";
        const char* citect_program_files_name = "citect-program-files";
        p.add<std::string>(backup_name, 'b', "path to backup folder", false, "");
        p.add<std::string>(citect_project_path_name, 'c', "path to citect project to zip, please also provide citect-program-files path if available", false, "");
        p.add<std::string>(citect_program_files_name, 'p', "path to citect program files to zip, must provide citect-project-path as well", false, "");
        if (argc == -1)
            p.parse_check(*argv);
        else
            p.parse_check(argc, argv);

        const std::string& backup_path = p.get<std::string>(backup_name);
        const std::string& citect_project_folder_string = p.get<std::string>(citect_project_path_name);
        const std::string& citect_program_files_string = p.get<std::string>(citect_program_files_name);
        if (citect_project_folder_string.size())
        {
            Path project = citect_project_folder_string;
            Path program_files = citect_program_files_string;
            if (fs::exists(project))
            {
                //TODO: Do Backup

                CitectData cd = {
                    .project_path = project,
                    .program_files_path = fs::exists(program_files) ? program_files : Path(),
                    .backup_path = backup_path,
                };
                RunCitectCreateZipJob* job = new RunCitectCreateZipJob();
                job->m_citect_data = &cd;
                Threading::GetInstance().SubmitJob(job);
                while (cd.total == 0)
                {
                    SysSleep(200);
                }

                do {
                    TuiProgressBar(cd.progress, cd.total);
                    SysSleep(100);
                } while (cd.total != 0);
                TuiProgressBar(100, 100);
            }
            else
            {
                std::cerr << "folder does not exist for citect-zip: " << citect_project_folder_string;
            }
        }
        return 0;
    }
    HideConsole();
#endif


    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100 (WebGL 1.0)
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
    // GL ES 3.0 + GLSL 300 es (WebGL 2.0)
    const char* glsl_version = "#version 300 es";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context

    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    float normalRatio = 16.0f / 9.0f;
    Vec2I screen_size = { mode->width, mode->height };
    float displayRatio = float(mode->width) / float(mode->height);
    Vec2I window_size = {};
#if 1
    //window_size = Vec2I(1280, 720);
    window_size = Vec2I(1024, 600);
#else
    float screen_scale = 1.5;
    if (displayRatio < normalRatio)
    {
        window_size.x  = i32(float(mode->width) / screen_scale);
        window_size.y = i32(float(mode->width) / normalRatio);
    }
    else
    {
        window_size.y = i32(mode->height / screen_scale);
        window_size.x  = i32(normalRatio * window_size.y);
    }
#endif

    float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor()); // Valid on GLFW 3.3+ only
    //GLFWwindow* window = glfwCreateWindow((int)(monitor_size.x * main_scale), (int)(monitor_size.y * main_scale), "Quool Tool", nullptr, nullptr);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    gfx.window = glfwCreateWindow(window_size.x, window_size.y, "Quool Tool", nullptr, nullptr);
    if (gfx.window == nullptr)
        return 1;
    const Vec2I win_p = (screen_size - window_size) / 2;
    glfwSetWindowPos(gfx.window, win_p.x, win_p.y);
    glfwMakeContextCurrent(gfx.window);
    glfwSwapInterval(1); // Enable vsync

    OSInit(gfx.window);
    Threading& threading = Threading::GetInstance();

    NetworkingInit();

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.IniFilename = NULL;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(gfx.window, true);
#ifdef __EMSCRIPTEN__
    ImGui_ImplGlfw_InstallEmscriptenCallbacks(gfx.window, "#canvas");
#endif
    ImGui_ImplOpenGL3_Init(glsl_version);

    ThemesInit();
    ThemeSetColor(g_data.settings.color);
    ThemeSetStyle(g_data.settings.style);

#if _DEBUG
    {
        std::ifstream file(g_settings_filename);
        bool file_exists = file.good();
        file.close();
        if (file_exists)
        {
            ReadSettings(&g_data.settings, g_settings_filename);
            ThemeSetColor(g_data.settings.color);
            ThemeSetStyle(g_data.settings.style);
        }
        else
        {
            WriteSettings(&g_data.settings, g_settings_filename);
            ThemeSetColor(g_data.settings.color);
            ThemeSetStyle(g_data.settings.style);
        }
    }
#endif

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details. If you like the default font but want it to scale better, consider using the 'ProggyVector' from the same author!
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
    style.FontSizeBase = 16.0f;
    g_data.fonts[FontIndex_Default] = LoadFontForImgui(IDR_FONT1, 16.0f);
    g_data.fonts[FontIndex_Imgui] = io.Fonts->AddFontDefault();
    g_data.fonts[FontIndex_Monospace] = LoadFontForImgui(IDR_FONT2, 16.0f);

    // Our state
    bool keepProcessWindowAlive = true;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    u64 frameStartTicks = 0;

    // Main loop
#ifdef __EMSCRIPTEN__
    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    //while (!glfwWindowShouldClose(gfx.window))
#endif

    AppData app_data;

    glfwShowWindow(gfx.window);
    bool done = false;
    while (!(done || glfwWindowShouldClose(gfx.window)))
    {
        {
            ZoneScopedN("Frame Update:");
            // Poll and handle events (inputs, window resize, etc.)
            // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
            // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
            // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
            // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
            {
                ZoneScopedN("Poll Events");
                glfwPollEvents();
            }

            //if (glfwGetWindowAttrib(gfx.window, GLFW_ICONIFIED) != 0)
            //{
            //    ImGui_ImplGlfw_Sleep(10);
            //    continue;
            //}

#if _DEBUG
            if (glfwGetKey(gfx.window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            {
                done = true;
            }
#endif

            {
                ZoneScopedN("ImGui Create New Frame");
                // Start the Dear ImGui frame
                {
                    ZoneScopedN("ImGui OpenGL3 New Frame");
                    ImGui_ImplOpenGL3_NewFrame();
                }
                {
                    ZoneScopedN("ImGui ImplGlfw New Frame");
                    ImGui_ImplGlfw_NewFrame();
                }
                {
                    ZoneScopedN("ImGui New Frame");
                    ImGui::NewFrame();
                }
            }

            {
                ZoneScopedN("Main Imgui");
                ImguiMain(app_data);
            }


            {
                ZoneScopedN("ImGui Render");
                {
                    ZoneScopedN("ImGui Render");
                    ImGui::Render();
                }
                glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
                glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
                glClear(GL_COLOR_BUFFER_BIT);
                {
                    ZoneScopedN("RenderDrawData");
                    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
                }
            }
        }

        {
            ZoneScopedN("Frame End");
            glfwSwapBuffers(gfx.window);
        }
        FrameMark;
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(gfx.window);
    glfwTerminate();

    return 0;
}