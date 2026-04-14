#include "ImguiHelper.h"
#include "Tracy.hpp"

#include "System.h"
#include "WinInterop_File.h"
#include "Math.h"
#include "Threading.h"
#include "Themes.h"
#include "Settings.h"
#include "LoadJson.h"
#include "Citect.h"
#include "DataCollection.h"
#include "Version.h"
#include "Networking.h"
#include "Rendering.h"

#include "SDL3/SDL.h"
#include "ImGui/backends/imgui_impl_sdl3.h"
#include "ImGui/backends/imgui_impl_sdlrenderer3.h"

#include <stdio.h>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>

bool s_show_demo_window = false;

bool ImguiInit()
{
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
    float main_scale = SysMonitorScale();
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForSDLRenderer(gfx.window, gfx.context);
    ImGui_ImplSDLRenderer3_Init(gfx.context);
    return true;
}

void ImguiDestroy()
{
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

void ImguiProcessEvent(const SDL_Event* event)
{
    ImGui_ImplSDL3_ProcessEvent(event);
}

void ImguiNewFrame()
{
    ZoneScoped;
    {
        ZoneScopedN("ImGui SDL Renderer3 New Frame");
        ImGui_ImplSDLRenderer3_NewFrame();
    }
    {
        ZoneScopedN("ImGui SDL3 New Frame");
        ImGui_ImplSDL3_NewFrame();
    }
    {
        ZoneScopedN("ImGui New Frame");
        ImGui::NewFrame();
    }
}

void ImguiText(const std::wstring& ws)
{
    std::string s;
    SysConvertWideCharToMultiByte(s, ws);
    ImGui::TextUnformatted(s.c_str());
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
    SysConvertWideCharToMultiByte(s, ws);
    bool r = ImGui::InputText(title.c_str(), s.data(), s.capacity(), flags | ImGuiInputTextFlags_CallbackResize, DynamicTextCallback, &s);
    SysConvertMultibyteToWideChar(ws, s);
    if (ws[ws.size() - 1] == 0)
        ws.pop_back();
    return r;
}
bool InputTextMultilineDynamicSize(const std::string& title, std::string& s, ImGuiInputTextFlags flags = ImGuiInputTextFlags_None)
{
    return ImGui::InputTextMultiline(title.c_str(), const_cast<char*>(title.data()), s.capacity(), ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 2), flags | ImGuiInputTextFlags_CallbackResize, DynamicTextCallback, &s);
}
bool InputTextDynamicSize(const std::string& title, Path& path, ImGuiInputTextFlags flags = ImGuiInputTextFlags_None)
{
    std::string s;
    SysConvertWideCharToMultiByte(s, path.wstring());
    bool r = ImGui::InputText(title.c_str(), s.data(), s.capacity(), flags | ImGuiInputTextFlags_CallbackResize, DynamicTextCallback, &s);
    if (s.size() && s.back() == '\0')
        path = s.substr(0, s.size() - 1);// remove nullterminator
    else
        path = s;
    return r;
}

void ImguiTextCentered(const std::string& text, const Color* color)
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
    if (color)
        ImGui::TextColored(ToImguiColor(*color), text.c_str());
    else
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

bool ImguiPath(const std::string& name, const std::string& hint, Path& out_path)
{
    ImGui::PushID(name.c_str());
    ImGui::Text(name.c_str());
    ImGui::SameLine();
    HelpMarker(hint.c_str());
    ImGui::SameLine();
    //ImGui::PushItemWidth(-FLT_MIN);
    bool modified = InputTextDynamicSize("##" + hint, out_path);
    //ImGui::PopItemWidth();
    //TODO:
    //ImGui::SameLine();
    //if (ImGui::Button("..."))
    {
        //not setup
    }
    ImGui::PopID();
    return modified;
}

void ImguiMain(AppData& data)
{
    ZoneScoped;
    Threading& threading = Threading::GetInstance();

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
        //ImGuiWindowFlags_MenuBar |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoMove;

    if (ImGui::Begin("Main", nullptr, windowFlags))
    {
        ZoneScopedN("Main");

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("Version"))
            {
                ZoneScopedN("Version");

                const bool new_version = g_online_version.IsValid() && g_online_version > g_version;
                if (new_version)
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.1f, 0.1f, 1.0f), "Version: %s", g_version.AsString().c_str());
                    if (g_download_state == AsyncStatus_Fetching)
                    {
                        if (g_download_update_progress >= 0.0f)
                            ImGui::ProgressBar(g_download_update_progress, ImVec2(-FLT_MIN, 20));
                        else
                            ImGui::ProgressBar(-1.0f * (float)ImGui::GetTime(), ImVec2(-FLT_MIN, 20));
                    }
                    else if (g_download_state == AsyncStatus_FetchedSuccess)
                    {
                        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.1f, 1.0f), "New version downloaded close and run the new version");
                    }
                    else
                    {
                        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.1f, 1.0f), "New Version Available!");
                        ImGui::BeginDisabled(g_download_state != AsyncStatus_Empty);
                        const std::string bs = ToString("Download Version %s", g_online_version.AsString().c_str());
                        if (ImGui::Button(bs.c_str()))
                        {
                            DownloadUpdateJob* job = new DownloadUpdateJob();
                            threading.SubmitJob(job);
                        }
                        ImGui::EndDisabled();
                    }
                }
                else
                {

                    ImGui::Text("Version: %s", g_version.AsString().c_str());
                    if (g_online_version.IsValid())
                    {
                        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Already at latest version");
                    }
                    ImGui::BeginDisabled(g_version_state != AsyncStatus_Empty || g_online_version.IsValid());
                    if (ImGui::Button("Check for update"))
                    {
                        GetOnlineVersionJob* job = new GetOnlineVersionJob();
                        threading.SubmitJob(job);
                    }
                    ImGui::EndDisabled();
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("About"))
            {
                ZoneScopedN("About");

                ImGui::Text("Color:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(100);
                i32 colorSelection = g_theme_settings.color;

                if (ImGui::Combo("##Color", (i32*)&g_theme_settings.color, GetCStringFromThemes, &g_ThemeColorOptions, (i32)ThemeColor_Count))
                {
                    if (colorSelection != g_theme_settings.color)
                    {
                        ThemeSetColor(g_theme_settings.color);
                        WriteSettings();
                    }
                }
                ImGui::Text("Style:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(100);
                i32 styleSelection = g_theme_settings.style;
                if (ImGui::Combo("##Style", (i32*)&g_theme_settings.style, GetCStringFromThemes, &g_ThemeStyleOptions, (i32)ThemeStyle_Count))
                {
                    if (styleSelection != g_theme_settings.style)
                    {
                        ThemeSetStyle(g_theme_settings.style);
                        WriteSettings();
                    }
                }

                if (ImGui::MenuItem("Github Releases"))
                    SysRunShellProcess(L"https://github.com/CharlesHenryVIII/QuoolTool/releases", nullptr);
#if _DEBUG
                if (ImGui::MenuItem("imgui demo"))
                    s_show_demo_window = !s_show_demo_window;
#endif
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }


        ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_NoCloseWithMiddleMouseButton | ImGuiTabBarFlags_FittingPolicyScroll | ImGuiTabBarFlags_DrawSelectedOverline;
        if (ImGui::BeginTabBar("MyTabBar", tab_bar_flags))
        {
            if (ImGui::BeginTabItem("Data Collection"))
            {
                DataCollectionImGui(*data.data_collection_data);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Network"))
            {
                NetworkImgui(*data.network_data);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Citect/AVEVA"))
            {
                CitectImGui(*data.citect_data);
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }




        ImGui::End();
    }

    if (s_show_demo_window)
        ImGui::ShowDemoWindow(&s_show_demo_window);
}

void ImguiCenterWrappedText(const char* text, float start_position, float wrap_width)
{
    ImFont* font = ImGui::GetFont();
    float font_size = ImGui::GetFontSize();

    const char* line_start = text;
    while (*line_start)
    {
        const char* line_end = font->CalcWordWrapPosition(font_size, line_start, text + strlen(text), wrap_width);
        std::string line(line_start, line_end);
        ImGui::SetCursorPosX(start_position);
        ImGui::TextAligned(0.5f, wrap_width, line.c_str());

        if (line_end == line_start)
            break;
        line_start = line_end;
        while (*line_start == ' ')
            line_start++;
    }
}

void ImguiAlignForWidth(float width, float alignment)
{
    ImGuiStyle& style = ImGui::GetStyle();
    float avail = ImGui::GetContentRegionAvail().x;
    float off = (avail - width) * alignment;
    if (off > 0.0f)
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);
}

void ImguiDrawDashedLine(ImDrawList* drawList, ImVec2 p1, ImVec2 p2, ImU32 col, float thickness, float dash_len, float dash_gap)
{
    ImVec2 dir = ImVec2(p2.x - p1.x, p2.y - p1.y);
    float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
    
    // Normalize direction
    if (len > 0.0f)
    {
        dir.x /= len; 
        dir.y /= len;
    }

    for (float t = 0; t < len; t += dash_len + dash_gap)
    {
        float t_end = t + dash_len;
        if (t_end > len) t_end = len; // Clamp to end of line
        drawList->AddLine(
            ImVec2(p1.x + dir.x * t, p1.y + dir.y * t),
            ImVec2(p1.x + dir.x * t_end, p1.y + dir.y * t_end),
            col, thickness);
    }
}

void ImguiDrawDashedRect(ImDrawList* drawList, ImVec2 p_min, ImVec2 p_max, ImU32 col, float thickness, float dash_len, float dash_gap)
{
    ImVec2 tr(p_max.x, p_min.y); // Top Right
    ImVec2 bl(p_min.x, p_max.y); // Bottom Left
    
    ImguiDrawDashedLine(drawList, p_min, tr, col, thickness, dash_len, dash_gap); // Top
    ImguiDrawDashedLine(drawList, tr, p_max, col, thickness, dash_len, dash_gap); // Right
    ImguiDrawDashedLine(drawList, p_max, bl, col, thickness, dash_len, dash_gap); // Bottom
    ImguiDrawDashedLine(drawList, bl, p_min, col, thickness, dash_len, dash_gap); // Left
}

bool ImguiEdit(SysIP4* a, bool align_right)
{
    const u8 step = 1;
    const u8 fast_step = 16;
    bool edited = false;



    const float label_size = ImGui::CalcTextSize("255", NULL, true).x;
    const float width = label_size + ImGui::GetStyle().ItemInnerSpacing.x + 5.0f;
    if (align_right)
    {
        const float window_visible_x = ImGui::GetWindowContentRegionMax().x;
        const float cursor_pos_x = window_visible_x - (4 * 1) - (4 * width) - (4 * ImGui::GetStyle().ItemInnerSpacing.x);
        ImGui::SetCursorPosX(cursor_pos_x);
    }

    const ImVec2 default_item_spacing = ImGui::GetStyle().ItemSpacing;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1, default_item_spacing.y));
    ImGui::PushItemWidth(width);
    edited |= ImGui::InputScalar("##IPv4_a", ImGuiDataType_U8, &(a->a));
    ImGui::SameLine();
    ImGui::Text(".");
    ImGui::SameLine();
    ImGui::PushItemWidth(width);
    edited |= ImGui::InputScalar("##IPv4_b", ImGuiDataType_U8, &(a->b));
    ImGui::SameLine();
    ImGui::Text(".");
    ImGui::SameLine();
    ImGui::PushItemWidth(width);
    edited |= ImGui::InputScalar("##IPv4_c", ImGuiDataType_U8, &(a->c));
    ImGui::SameLine();
    ImGui::Text(".");
    ImGui::SameLine();
    ImGui::PushItemWidth(width);
    edited |= ImGui::InputScalar("##IPv4_d", ImGuiDataType_U8, &(a->d));
    ImGui::PopStyleVar();
    return edited;
}
bool ImguiEdit(SysIP4Subnet* a)
{
    SysIP4 ip = a->ToIP4();
    bool edited = ImguiEdit(&ip, true);
    if (edited)
    {
        a->FromIP(ip);
    }
    return edited;
}
bool ImguiEdit(SysIP4AndSubnet* a)
{
    bool edited = false;
    ImGui::PushID("IPv4_IP");
    ImGui::Text("IP:");
    ImGui::SameLine();
    edited |= ImguiEdit(&a->ip, true);
    ImGui::PopID();

    ImGui::PushID("IPv4_Subnet");
    ImGui::Text("Subnet:");
    ImGui::SameLine();
    edited |= ImguiEdit(&a->subnet);
    ImGui::PopID();
    return edited;
}
//bool ImguiEdit(SysIP6& a)
//bool ImguiEdit(SysIP6AndSubnet& a)
//bool ImguiEdit(SysIP4Subnet& a)
//bool ImguiEdit(SysIP6Subnet& a)
//bool ImguiView(SysMacAddress& a)
