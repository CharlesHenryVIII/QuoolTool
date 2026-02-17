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

#include "WinInterop.h"
#include "WinInterop_File.h"
#include "Math.h"
#include "Threading.h"
#include "Themes.h"
#include "VideoData.h"
#include "LoadJson.h"

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

const wchar_t* settings_filename = L"settings.json";

void ImguiText(const std::wstring& ws)
{
    std::string s;
    ConvertWideCharToMultiByte(s, ws);
    ImGui::TextUnformatted(s.c_str());
}

[[nodiscard]] inline ImVec2 HadamardProduct(const ImVec2& a, const ImVec2& b)
{
    return { a.x * b.x, a.y * b.y };
}

int DynamicTextCallback(ImGuiInputTextCallbackData* data)
{
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
    {
        ASSERT(data->UserData);
        if (!data->UserData)
            return 1;
        std::string* string = (std::string*)data->UserData;
        string->resize(data->BufTextLen);
        data->Buf = string->data();
    }
    return 0;
}
bool InputTextDynamicSize(const std::string& title, std::wstring& ws, ImGuiInputTextFlags flags = ImGuiInputTextFlags_None)
{
    std::string s;
    ConvertWideCharToMultiByte(s, ws);
    bool r = ImGui::InputText(title.c_str(), s.data(), s.capacity(), flags | ImGuiInputTextFlags_CallbackResize, DynamicTextCallback, &s);
    ConvertMultibyteToWideChar(ws, s);
    if (ws[ws.size() - 1] == 0)
        ws.pop_back();
    return r;
}
bool InputTextMultilineDynamicSize(const std::string& title, std::string& s, ImGuiInputTextFlags flags = ImGuiInputTextFlags_None)
{
    return ImGui::InputTextMultiline(title.c_str(), const_cast<char*>(title.data()), s.capacity(), ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 2), flags | ImGuiInputTextFlags_CallbackResize, DynamicTextCallback, &s);
}

void TextCentered(std::string text)
{
    float win_width = ImGui::GetWindowSize().x;
    float text_width = ImGui::CalcTextSize(text.c_str()).x;

    // calculate the indentation that centers the text on one line, relative
    // to window left, regardless of the `ImGuiStyleVar_WindowPadding` value
    float text_indentation = (win_width - text_width) * 0.5f;

    // if text is too long to be drawn on one line, `text_indentation` can
    // become too small or even negative, so we check a minimum indentation
    float min_indentation = 20.0f;
    if (text_indentation <= min_indentation) {
        text_indentation = min_indentation;
    }

    ImGui::SameLine(text_indentation);
    ImGui::PushTextWrapPos(win_width - text_indentation);
    ImGui::Text(text.c_str());
    ImGui::PopTextWrapPos();
}


void CleanPathString(std::wstring& s, const bool add_final_slash)
{
    size_t pos = s.find(L'\\');
    while (pos != std::wstring::npos)
    {
        s.replace(pos, 1, L"/", 1);
        pos = s.find(L'\\');
    }
    if (add_final_slash && s.size() > 1 && s[s.size() - 1] != '/')
    {
        s.append(L"/");
    }
}

void HelpMarker(const std::string& desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc.c_str());
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

bool ImguiPath(const std::string& name, const std::string& hint, std::wstring& out_path, const bool add_final_slash)
{
    ImGui::PushID(name.c_str());
    ImGui::Text(name.c_str());
    ImGui::SameLine();
    HelpMarker(hint.c_str());
    ImGui::SameLine();
    //ImGui::PushItemWidth(-FLT_MIN);
    bool modified = InputTextDynamicSize("##" + hint, out_path);
    //ImGui::PopItemWidth();
    CleanPathString(out_path, add_final_slash);
    //TODO:
    //ImGui::SameLine();
    //if (ImGui::Button("..."))
    {
        //not setup
    }
    ImGui::PopID();
    return modified;
}

int Main(int, char**)
{
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
    float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor()); // Valid on GLFW 3.3+ only
    GLFWwindow* window = glfwCreateWindow((int)(1280 * main_scale), (int)(800 * main_scale), "SCADA Backup", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    InitOS(window);
    Threading& threading = Threading::GetInstance();

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
    ImGui_ImplGlfw_InitForOpenGL(window, true);
#ifdef __EMSCRIPTEN__
    ImGui_ImplGlfw_InstallEmscriptenCallbacks(window, "#canvas");
#endif
    ImGui_ImplOpenGL3_Init(glsl_version);

    InitOS(window);
    ThemesInit();
    {
        std::ifstream file(settings_filename);
        bool file_exists = file.good();
        file.close();
        if (file_exists)
        {
            ReadSettings(&g_data.settings, settings_filename);
            ThemeSetColor(g_data.settings.color);
            ThemeSetStyle(g_data.settings.style);
        }
        else
        {
            g_data.settings = {
                .mkv_path = L"C:/Program Files/MKVToolNix/mkvmerge.exe",
            };
            WriteSettings(&g_data.settings, settings_filename);
        }

    }

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details. If you like the default font but want it to scale better, consider using the 'ProggyVector' from the same author!
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
    //style.FontSizeBase = 20.0f;
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf");
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf");
    //IM_ASSERT(font != nullptr);

    // Our state
    bool show_demo_window = false;
    bool exitProgram = false;
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
    //while (!glfwWindowShouldClose(window))
#endif

    bool done = false;
    while (!(done || glfwWindowShouldClose(window)))
    {
        {
            ZoneScopedN("Frame Update:");
            // Poll and handle events (inputs, window resize, etc.)
            // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
            // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
            // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
            // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
            glfwPollEvents();
            if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
            {
                ImGui_ImplGlfw_Sleep(10);
                continue;
            }

            {
                ZoneScopedN("Create New Frame");
                // Start the Dear ImGui frame
                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();
                //ImGui::PushFont(mainFont);
            }
//#if 1
//            if (buildRunning && threading.GetJobsInFlight() == 0)
//            {
//                //BuildFinished
//                NotifyWindowBuildFinished();
//            }
//            buildRunning = threading.GetJobsInFlight();
//#else
//            if (!threading.GetJobsInFlight())
//                buildRunning = false;
//#endif

            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->WorkPos, ImGuiCond_Always, {});
            ImGui::SetNextWindowSize(viewport->WorkSize, ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(1.0f); // Transparent background
            ImGuiWindowFlags windowFlags =
                //ImGuiWindowFlags_NoBackground |
#if 0
                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse |
#else
                ImGuiWindowFlags_NoDecoration |
#endif
                ImGuiWindowFlags_MenuBar |
                ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoFocusOnAppearing |
                ImGuiWindowFlags_NoNav |
                ImGuiWindowFlags_NoMove;

            if (ImGui::Begin("Main", nullptr, windowFlags))
            {
                ZoneScopedN("Main");

                if (ImGui::BeginMenuBar())
                {
                    if (ImGui::BeginMenu("About"))
                    {
                        ZoneScopedN("About");
                        if (ImGui::MenuItem("Github Releases"))
                            RunProcess(L"https://github.com/CharlesHenryVIII/VideoWrapper/releases", nullptr, true);

                        ImGui::Text("Color:");
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(100);
                        i32 colorSelection = g_data.settings.color;

                        if (ImGui::Combo("##Color", (i32*)&g_data.settings.color, GetCStringFromThemes, &g_ThemeColorOptions, (i32)ThemeColor_Count))
                        {
                            if (colorSelection != g_data.settings.color)
                            {
                                ThemeSetColor(g_data.settings.color);
                                WriteSettings(&g_data.settings, settings_filename);
                            }
                        }
                        ImGui::Text("Style:");
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(100);
                        i32 styleSelection = g_data.settings.style;
                        if (ImGui::Combo("##Style", (i32*)&g_data.settings.style, GetCStringFromThemes, &g_ThemeStyleOptions, (i32)ThemeStyle_Count))
                        {
                            if (styleSelection != g_data.settings.style)
                            {
                                ThemeSetStyle(g_data.settings.style);
                                WriteSettings(&g_data.settings, settings_filename);
                            }
                        }
#if _DEBUG
                        if (ImGui::MenuItem("imgui demo"))
                            show_demo_window = !show_demo_window;
#endif
                        ImGui::EndMenu();
                    }
                    ImGui::EndMenuBar();
                }
                ImGuiWindowFlags sectionFlags =
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoSavedSettings |
                    ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoFocusOnAppearing |
                    ImGuiWindowFlags_NoMove;

                ImVec2 switchesScale = { 0, 0.17f };
                ImVec2 switchesSize = HadamardProduct(viewport->WorkSize, switchesScale);
                if (ImGui::BeginChild("File Paths", switchesSize, true, sectionFlags))
                {
                    ZoneScopedN("File Paths");
                    TextCentered("File Paths");
                    ImGui::NewLine();

                    std::wstring source_path = g_data.settings.source_path;
                    if (ImguiPath("Source", "Path to source video files", source_path, true))
                    {
                        if (source_path != g_data.settings.source_path && g_data.video_group.Clear())
                        {
                            g_data.settings.source_path = source_path;
                            WriteSettings(&g_data.settings, settings_filename);
                        }
                    }
                    if (ImguiPath("Destination", "Path to destination folder", g_data.settings.dest_path, true))
                        WriteSettings(&g_data.settings, settings_filename);
                    if (ImguiPath("mkvmerge", "Path to mkvmerge", g_data.settings.mkv_path, false))
                        WriteSettings(&g_data.settings, settings_filename);
                }
                ImGui::EndChild();

                ImVec2 videos_scale = { 0, 0.76f };
                ImVec2 videos_size = HadamardProduct(viewport->WorkSize, videos_scale);
                if (ImGui::BeginChild("Videos", videos_size, true, sectionFlags))
                {
                    ZoneScopedN("Videos");
                    TextCentered("Videos");
                    ImGui::NewLine();
                    if (g_data.video_group.thread_lock.try_lock())
                    {
                        Defer{ g_data.video_group.thread_lock.unlock(); };

                        //ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY))
                        const ImGuiTableFlags table_flags = ImGuiTableFlags_SizingFixedFit |
                                                            ImGuiTableFlags_NoSavedSettings |
                                                            ImGuiTableFlags_RowBg |
                                                            ImGuiTableFlags_Borders |
                                                            ImGuiTableFlags_Resizable |
                                                            ImGuiTableFlags_Reorderable |
                                                            ImGuiTableFlags_ScrollX |
                                                            ImGuiTableFlags_ScrollY |
                                                            ImGuiTableFlags_Hideable;

                        if (g_data.video_group.video_infos.size() && ImGui::BeginTable("Videos", g_data.video_group.max_tracks + 1, table_flags, ImVec2(videos_size.x, videos_size.y - 100)))
                        {
                            ImGui::TableSetupScrollFreeze(1, 0);
                            ImGui::TableSetupColumn("Name");
                            for (i32 i = 0; i < g_data.video_group.max_tracks; i++)
                            {
                                std::string name = ToString("Track %i", i);
                                ImGui::TableSetupColumn(name.c_str());
                            }
                            //ImGui::TableHeadersRow();

                            // Dummy entire-column selection storage
                            // FIXME: It would be nice to actually demonstrate full-featured selection using those checkbox.
                            static bool column_selected[3] = {};

                            // Instead of calling TableHeadersRow() we'll submit custom headers ourselves.
                            // (A different approach is also possible:
                            //    - Specify ImGuiTableColumnFlags_NoHeaderLabel in some TableSetupColumn() call.
                            //    - Call TableHeadersRow() normally. This will submit TableHeader() with no name.
                            //    - Then call TableSetColumnIndex() to position yourself in the column and submit your stuff e.g. Checkbox().)
                            ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
                            for (i32 column = 0; column < g_data.video_group.max_tracks + 1; column++)
                            {
                                ImGui::TableSetColumnIndex(column);
                                const char* column_name = ImGui::TableGetColumnName(column); // Retrieve name passed to TableSetupColumn()
                                ImGui::PushID(column);
                                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

                                bool all_true = true;
                                for (size_t i = 0; i < g_data.video_group.video_infos.size(); i++)
                                {
                                    if (column == 0)
                                    {
                                        if (!g_data.video_group.video_infos[i].encode)
                                        {
                                            all_true = false;
                                            break;
                                        }
                                    }
                                    else
                                    {
                                        if ((size_t)column <= g_data.video_group.video_infos[i].tracks.size() &&
                                            !g_data.video_group.video_infos[i].tracks[column - 1].encode)
                                        {
                                            all_true = false;
                                            break;
                                        }
                                    }
                                }

                                const char* bn = all_true ? "Disable All" : "Enable All";
                                if (ImGui::Button(bn))
                                {
                                    for (size_t i = 0; i < g_data.video_group.video_infos.size(); i++)
                                    {
                                        if (column == 0)
                                        {
                                            g_data.video_group.video_infos[i].encode = !all_true;
                                        }
                                        else
                                        {
                                            if ((size_t)column <= g_data.video_group.video_infos[i].tracks.size())
                                                g_data.video_group.video_infos[i].tracks[column - 1].encode = !all_true;
                                        }
                                    }
                                }
                                //ImGui::Checkbox("##checkall", &column_selected[column]);
                                ImGui::PopStyleVar();
                                ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
                                ImGui::TableHeader(column_name);
                                ImGui::PopID();
                            }

                            for (size_t vi = 0; vi < g_data.video_group.video_infos.size(); vi++)
                            {
                                VideoInfo& info = g_data.video_group.video_infos[vi];
                                std::string mb_name;
                                ConvertWideCharToMultiByte(mb_name, info.name);
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                ImGui::PushID(mb_name.c_str());
                                ImGui::Checkbox("##", &info.encode);
                                ImGui::SameLine();
                                ImguiText(info.name);

                                for (size_t i = 0; i < info.tracks.size(); i++)
                                {
                                    ImGui::TableSetColumnIndex(int(1 + i));
                                    Track& track = info.tracks[i];

                                    //std::string id_string = ToString("##%s %i", mb_name.c_str(), );
                                    ImGui::PushID(track.id);
                                    ImGui::Checkbox("##", &track.encode);
                                    ImGui::SameLine();
                                    ImGui::Text("%s %s", track.type.c_str(), track.details.c_str());
                                    ImGui::PopID();
                                }
                                ImGui::PopID();
                            }
                            ImGui::EndTable();
                        }

                        float height = 40;
                        if (g_data.settings.source_path.size() && ImGui::Button("SCAN", ImVec2(125, height)))
                        {
                            g_data.video_group.video_infos.clear();
                            g_data.video_group.max_tracks = 0;
                            RunUpdateVideoGroupJob* job = new RunUpdateVideoGroupJob();
                            job->mkv_path = g_data.settings.mkv_path;
                            job->source_path = g_data.settings.source_path;
                            job->dest_path = g_data.settings.dest_path;
                            job->video_group = &g_data.video_group;
                            threading.SubmitJob(job);
                        }
                        if (g_data.video_group.video_infos.size())
                        {
                            ImGui::SameLine();
                            if (ImGui::Button("ENCODE", ImVec2(125, height)))
                            {
                                RunEncodeJob* job = new RunEncodeJob();
                                job->mkv_path = g_data.settings.mkv_path;
                                job->source_path = g_data.settings.source_path;
                                job->dest_path = g_data.settings.dest_path;
                                job->video_group = &g_data.video_group;
                                threading.SubmitJob(job);
                            }
                        }
                        if (g_data.video_group.in_progress)
                        {
                            ImGui::SameLine();
                            ImVec2 size = ImVec2(-FLT_MIN, height);
                            if (!g_data.video_group.completed)
                                ImGui::ProgressBar(-1.0f * (float)ImGui::GetTime(), size, "Encoding...");
                            else
                                ImGui::ProgressBar(float(g_data.video_group.completed) / g_data.video_group.video_infos.size(), size);
                        }

                    }
                    else
                    {
                        ImGui::ProgressBar(-1.0f * (float)ImGui::GetTime(), ImVec2(-FLT_MIN, 40), "SCANNING ...");
                    }
                }
                ImGui::EndChild();

                if (exitProgram)
                {
                    done = true;
                }

                ImGui::End();
            }


            if (show_demo_window)
                ImGui::ShowDemoWindow(&show_demo_window);

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
        FrameMark;


        {
            ZoneScopedN("Frame End");
            glfwSwapBuffers(window);
        }
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}