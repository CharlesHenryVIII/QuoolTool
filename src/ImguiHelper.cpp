#include "ImguiHelper.h"
#include "Tracy.hpp"

#include "WinInterop.h"
#include "WinInterop_File.h"
#include "Math.h"
#include "Threading.h"
#include "Themes.h"
#include "Settings.h"
#include "LoadJson.h"
#include "Citect.h"

#include <stdio.h>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>

const wchar_t* g_settings_filename = L"settings.json";
bool s_show_demo_window = false;

void ImguiText(const std::wstring& ws)
{
    std::string s;
    ConvertWideCharToMultiByte(s, ws);
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
bool InputTextDynamicSize(const std::string& title, Path& path, ImGuiInputTextFlags flags = ImGuiInputTextFlags_None)
{
    std::string s;
    ConvertWideCharToMultiByte(s, path.wstring());
    bool r = ImGui::InputText(title.c_str(), s.data(), s.capacity(), flags | ImGuiInputTextFlags_CallbackResize, DynamicTextCallback, &s);
    path = s;
    return r;
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

void ImguiMain()
{
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
                    RunProcess(L"https://github.com/CharlesHenryVIII/ScadaBackup/releases", nullptr, true);

                ImGui::Text("Color:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(100);
                i32 colorSelection = g_data.settings.color;

                if (ImGui::Combo("##Color", (i32*)&g_data.settings.color, GetCStringFromThemes, &g_ThemeColorOptions, (i32)ThemeColor_Count))
                {
                    if (colorSelection != g_data.settings.color)
                    {
                        ThemeSetColor(g_data.settings.color);
                        WriteSettings(&g_data.settings, g_settings_filename);
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
                        WriteSettings(&g_data.settings, g_settings_filename);
                    }
                }
#if _DEBUG
                if (ImGui::MenuItem("imgui demo"))
                    s_show_demo_window = !s_show_demo_window;
#endif
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }



        CitectImGui();


        ImGui::End();
    }

    if (s_show_demo_window)
        ImGui::ShowDemoWindow(&s_show_demo_window);
}


