#include "Tools.h"
#include "String.h"
#include "ImguiHelper.h"
#include "Tracy.hpp"
#include "LoadJson.h"
#include "WinInterop.h"

struct ScriptInfo {
    std::string name;
    bool selected;
    //function
};

ScriptInfo s_scripts[] = {
    { .name = "NETSTAT",    .selected = true, },
    { .name = "SYSINFO",    .selected = true, },
    { .name = "IPCONFIG",   .selected = true, },
};

void ToolsImGui(ToolsData& td)
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    Threading& threading = Threading::GetInstance();
    ImGuiWindowFlags sectionFlags =
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoMove;
    
    if (ImGui::BeginChild("File Paths", { 0, 90 }, true, sectionFlags))
    {
        ZoneScopedN("File Paths");
        TextCentered("File Paths");
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

    #define TITLE_NAME "Scripts"
    const ImVec2 main_scale = { 0, 0 };
    const ImVec2 main_size = HadamardProduct(viewport->WorkSize, main_scale);
    if (ImGui::BeginChild(TITLE_NAME, main_size, true, sectionFlags))
    {
        ZoneScopedN(TITLE_NAME);
        TextCentered(TITLE_NAME);
        ImGui::NewLine();

        std::error_code ec;
        ImGui::BeginDisabled(td.running);
        float height = 40;
        ImVec2 button_size(150.0f, 75.0f);
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
                s.selected = !s.selected;

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
            if (!s.selected)
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

        if (td.running)
        {
            ImGui::SameLine();
            if (td.progress == u64(-1))
            {
                ImGui::ProgressBar(-1.0f * (float)ImGui::GetTime(), ImVec2(-FLT_MIN, height), "Getting Data...");
            }
            else
            {
                ImGui::ProgressBar(float(td.progress) / td.total, ImVec2(-FLT_MIN, height));
            }
        }
    }
    ImGui::EndChild();
}
