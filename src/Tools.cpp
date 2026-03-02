#include "Tools.h"
#include "String.h"
#include "ImguiHelper.h"
#include "Tracy.hpp"
#include "LoadJson.h"
#include "WinInterop.h"

struct ScriptInfo {
    std::string name;
    Atomic<ScriptInfoFlags> flags;
    std::wstring cmdline; //function
    //break
    Atomic<bool> completed = false;
};

ScriptInfo s_scripts[] = {
    { .name = "SYSINFO",    .flags = ScriptInfoFlags(ScriptInfoFlags_Enabled),   .cmdline = L"systeminfo"},
    { .name = "NETSTAT",    .flags = ScriptInfoFlags(ScriptInfoFlags_Enabled),   .cmdline = L"netstat -ano" },
    { .name = "IPCONFIG",   .flags = ScriptInfoFlags(ScriptInfoFlags_Enabled),   .cmdline = L"ipconfig" },
    { .name = "PROGRAMS",   .flags = ScriptInfoFlags(ScriptInfoFlags_Enabled),   .cmdline = L"powershell -command \"Get-ItemProperty 'HKLM:/Software/Microsoft/Windows/CurrentVersion/Uninstall/*' | Where {$_.DisplayName} | Select DisplayName,DisplayVersion\"" },
    { .name = "PROCESSOR",  .flags = ScriptInfoFlags(ScriptInfoFlags_Enabled),   .cmdline = L"powershell -command \"Get-CimInstance Win32_Processor | Select-Object Name, NumberOfCores, NumberOfLogicalProcessors, MaxClockSpeed\"" },

};

std::string s_log;

void ImguiLog(const std::string& s)
{
    DebugPrint(s.c_str());
    s_log += s + "\n";
}
void ImguiLog(const std::wstring& ws)
{
    std::string s;
    ConvertWideCharToMultiByte(s, ws);
    ImguiLog(s);
}

void ToolsImGui(ToolsData& td)
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    Threading& threading = Threading::GetInstance();
    ImGuiWindowFlags section_flags =
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoMove;
    
    #define FILES_TITLE "File Paths"
    if (ImGui::BeginChild(FILES_TITLE, { 0, 90 }, true, section_flags))
    {
        ZoneScopedN(FILES_TITLE);
        TextCentered(FILES_TITLE);
        ImGui::NewLine();

        {
            bool locked = td.lock.try_lock();
            Defer{ td.lock.unlock(); };
            ImGui::BeginDisabled(!locked);
            ImGui::BeginGroup();
            if (ImguiPath("Output Folder", "Please select the folder to output data to", td.output_path))
                WriteSettings(&g_data.settings, g_settings_filename);
            ImGui::EndGroup();
            ImGui::EndDisabled();
        }


    }
    ImGui::EndChild();

    #define SCRIPTS_TITLE "Scripts"
    if (ImGui::BeginChild(SCRIPTS_TITLE, { 0, 125 }, true, section_flags))
    {
        ZoneScopedN(SCRIPTS_TITLE);
        TextCentered(SCRIPTS_TITLE);
        ImGui::NewLine();

        std::error_code ec;
        ImGui::BeginDisabled(td.running);
        float height = 40;
        const ImVec2 button_size(125.0f, 60.0f);
        ImGui::SetCursorPosX(Max((ImGui::GetContentRegionAvail().x - (arrsize(s_scripts) * button_size.x)) * 0.5f, ImGui::GetStyle().ItemSpacing.x));
        const float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
        for (i32 i = 0; i < arrsize(s_scripts); i++)
        {
            ScriptInfo& s = s_scripts[i];
            ImGui::PushID(i);
            ImGui::BeginGroup();

            bool pressed = ImGui::InvisibleButton("##btn", button_size);
            bool hovered = ImGui::IsItemHovered();
            bool active = ImGui::IsItemActive();
            if (pressed)
                FlagToggle(s.flags, ScriptInfoFlags_Enabled);

            ImVec2 p_min = ImGui::GetItemRectMin();
            ImVec2 p_max = ImGui::GetItemRectMax();

            ImDrawList* draw = ImGui::GetWindowDrawList();
            ImU32 background_color = ImGui::GetColorU32(ImGuiCol_Button);
            if (active)
                background_color = ImGui::GetColorU32(ImGuiCol_ButtonActive);
            if (hovered)
                background_color = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
            draw->AddRectFilled(p_min, p_max, background_color, 6.0f);
            draw->AddRect(p_min, p_max, ImGui::GetColorU32(ImGuiCol_Border), 6.0f);

            ImVec2 title_size = ImGui::CalcTextSize(s.name.c_str());
            ImVec2 title_pos = {
                p_min.x + (button_size.x - title_size.x) * 0.5f,
                p_min.y + 6.0f
            };
            draw->AddText(title_pos, ImGui::GetColorU32(ImGuiCol_Text), s.name.c_str());

            std::string selected_text = "Enabled";
            ImU32 selected_color = IM_COL32(0, 255, 0, 255);
            if (!FlagExists(s.flags, ScriptInfoFlags_Enabled))
            {
                selected_text = "Disabled";
                selected_color = IM_COL32(255, 0, 0, 255);
            }
            ImVec2 value_size = ImGui::CalcTextSize(selected_text.c_str());
            ImVec2 value_pos = {
                p_min.x + (button_size.x - value_size.x) * 0.5f,
                p_min.y + (button_size.y - value_size.y) * 0.5f + 6.0f
            };
            draw->AddText(value_pos, selected_color, selected_text.c_str());

            ImGui::EndGroup();
            ImGui::PopID();

            float last_button_x2 = ImGui::GetItemRectMax().x;
            float next_button_x2 = last_button_x2 + ImGui::GetStyle().ItemSpacing.x + button_size.x; // Expected position if next button was on same line

            float text_start = ImGui::GetCursorPosX() + ImGui::GetStyle().ItemSpacing.x / 2;
            if (i + 1 < arrsize(s_scripts) && next_button_x2 < window_visible_x2)
                ImGui::SameLine();
        }
        ImGui::EndDisabled();
    }
    ImGui::EndChild();



    #define ACTION_TITLE "Action"
    const ImVec2 action_scale = { 0.15f, 0 };
    const ImVec2 action_size = HadamardProduct(viewport->WorkSize, action_scale);
    if (ImGui::BeginChild(ACTION_TITLE, action_size, true, section_flags))
    {
        ZoneScopedN(ACTION_TITLE);

        ImGui::BeginDisabled(td.running || (!td.output_path.empty() && !fs::exists(td.output_path)));
        const ImVec2 avail = ImGui::GetContentRegionAvail();
        if (ImGui::Button("Run Scripts", avail))
        {
            ImguiLog("Running Scripts:");
            td.running = true;
            for (i32 i = 0; i < arrsize(s_scripts); i++)
            {
                ScriptInfo& s = s_scripts[i];
                if (!FlagExists(s.flags, ScriptInfoFlags_Enabled))
                    continue;
                RunProcessLogToFileJob* job = new RunProcessLogToFileJob();
                job->application_path;
                job->arguments = s.cmdline;
                const std::string name = s.name + ".txt";
                Path output_file = name;
                if (fs::exists(td.output_path))
                     output_file = Path(td.output_path / name);
                job->output_file = output_file;
                job->completed = &s.completed;
                threading.SubmitJob(job);

                ImguiLog(ToString("Running: %s", s.name.c_str()));
            }
        }
        ImGui::EndDisabled();
        
    }
    ImGui::EndChild();


#if 1
    ImGui::SameLine();
    #define LOG_TITLE "Progress"
    const ImVec2 progress_scale = { 0, 0 };
    ImVec2 progress_size = HadamardProduct(viewport->WorkSize, progress_scale);
    if (ImGui::BeginChild(LOG_TITLE, progress_size, ImGuiChildFlags_Borders, section_flags))
    {
        ZoneScopedN(LOG_TITLE);
        TextCentered(LOG_TITLE);
        //ImGui::Separator();

        if (td.running)
        {
            const ImVec2 ip_scale = { 0, 0.75 };
            ImVec2 ip_size = HadamardProduct(ImGui::GetContentRegionAvail(), ip_scale);
            if (ImGui::BeginChild("IndividualProgress", ip_size, ImGuiChildFlags_Borders, section_flags))
            {
                const float individual_height = 40.0f;
                for (i32 i = 0; i < arrsize(s_scripts); i++)
                {
                    ScriptInfo& s = s_scripts[i];
                    if (!FlagExists(s.flags, ScriptInfoFlags_Enabled))
                        continue;

                    if (s.completed)
                    {
                        ImGui::ProgressBar(1.0f, ImVec2(-FLT_MIN, individual_height), s.name.c_str());
                    }
                    else
                    {
                        ImGui::ProgressBar(-1.0f * (float)ImGui::GetTime(), ImVec2(-FLT_MIN, individual_height), s.name.c_str());
                    }

                }
            }
            ImGui::EndChild();

            float progress_bar_height = 50;
            ImVec2 max = ImGui::GetWindowContentRegionMax();
            ImGui::SetCursorPosY(max.y - progress_bar_height);
  
            i32 total = 0;
            i32 completed = 0;
            for (i32 i = 0; i < arrsize(s_scripts); i++)
            {
                if (!FlagExists(s_scripts[i].flags, ScriptInfoFlags_Enabled))
                    continue;
                total++;
                if (s_scripts[i].completed)
                    completed++;
            }
            if (total == completed)
                ImGui::ProgressBar(1.0f, ImVec2(-FLT_MIN, progress_bar_height), "Completed");
            else
            {
                std::string title = ToString("%i/%i", completed, total);
                ImGui::ProgressBar(float(completed) / float(total), ImVec2(-FLT_MIN, progress_bar_height), title.c_str());
            }
        }
    }
    ImGui::EndChild();
#else

    ImGui::SameLine();
    #define LOG_TITLE "Log"
    const ImVec2 log_scale = { 0, 0 };
    ImVec2 log_size = HadamardProduct(viewport->WorkSize, log_scale);
    if (ImGui::BeginChild(LOG_TITLE, log_size, ImGuiChildFlags_Borders, section_flags))
    {
        ZoneScopedN(LOG_TITLE);
        TextCentered(LOG_TITLE);
        ImGui::Separator();

        ImDrawList* draw = ImGui::GetWindowDrawList();
        //Screen-space position of the content area
        ImVec2 area_min = ImGui::GetCursorScreenPos();
        ImVec2 area_size = ImGui::GetContentRegionAvail();
        ImVec2 area_max = ImVec2(area_min.x + area_size.x, area_min.y + area_size.y);

        //Clip to child window and add black console background
        draw->PushClipRect(area_min, area_max, true);
        draw->AddRectFilled(area_min, area_max, IM_COL32(0, 0, 0, 255), 6.0f, ImDrawFlags_RoundCornersNone);
        draw->PopClipRect();

        //Padding inside the log area
        ImGui::SetCursorScreenPos(area_min + ImVec2(8, 6));
        ImGui::PushFont(g_data.fonts[FontIndex_Monospace]);
#if 1
        ImGui::TextUnformatted(s_log.c_str());
#else
        ImGui::TextUnformatted("This is a test of the monospace font");
#endif
        ImGui::PopFont();

    }
    ImGui::EndChild();
#endif
}
