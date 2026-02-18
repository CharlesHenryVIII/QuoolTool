#include "ImguiHelper.h"
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

const wchar_t* g_settings_filename = L"settings.json";
bool s_show_demo_window = false;

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


void Citect()
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    Threading& threading = Threading::GetInstance();
    SettingsCitect& citect_settings = g_data.settings.citect;
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


        if (ImguiPath("Program Data", "C:/ProgramData/AVEVA/Citect SCADA 2018 R2", citect_settings.program_data_path, true))
            WriteSettings(&g_data.settings, g_settings_filename);
        if (ImguiPath("Program Files", "C:/Program Files/AVEVA/Citect SCADA 208 R2", citect_settings.program_files_path, true))
            WriteSettings(&g_data.settings, g_settings_filename);
        if (ImguiPath("Deployment", "%PROGRAMDATA%/AVEVA Plant SCADA/Deployment", citect_settings.deployment_path, true))
            WriteSettings(&g_data.settings, g_settings_filename);
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
            //if (g_data.settings.source_path.size() && ImGui::Button("SCAN", ImVec2(125, height)))
            //{
            //    g_data.video_group.video_infos.clear();
            //    g_data.video_group.max_tracks = 0;
            //    RunUpdateVideoGroupJob* job = new RunUpdateVideoGroupJob();
            //    job->mkv_path = g_data.settings.mkv_path;
            //    job->source_path = g_data.settings.source_path;
            //    job->dest_path = g_data.settings.dest_path;
            //    job->video_group = &g_data.video_group;
            //    threading.SubmitJob(job);
            //}
            //if (g_data.video_group.video_infos.size())
            //{
            //    ImGui::SameLine();
            //    if (ImGui::Button("ENCODE", ImVec2(125, height)))
            //    {
            //        RunEncodeJob* job = new RunEncodeJob();
            //        job->mkv_path = g_data.settings.mkv_path;
            //        job->source_path = g_data.settings.source_path;
            //        job->dest_path = g_data.settings.dest_path;
            //        job->video_group = &g_data.video_group;
            //        threading.SubmitJob(job);
            //    }
            //}
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



        Citect();


        ImGui::End();
    }

    if (s_show_demo_window)
        ImGui::ShowDemoWindow(&s_show_demo_window);
}
