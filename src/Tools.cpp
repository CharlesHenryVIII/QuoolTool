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
    { .name = "netstat",    .selected = false, },
    { .name = "sysinfo",    .selected = false, },
    { .name = "ipconfig",   .selected = false, },
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
        for (i32 i = 0; i < arrsize(s_scripts); i++)
        {
            ScriptInfo& s = s_scripts[i];
            ImGui::PushID(i);
            ImGui::BeginGroup();

            ImVec2 size(200.0f, 100.0f);
            if (ImGui::InvisibleButton("bigbutton", size))
            {
                s.selected = !s.selected;
            }

            bool hovered = ImGui::IsItemHovered();
            bool clicked = ImGui::IsItemClicked();

            ImDrawList* draw = ImGui::GetWindowDrawList();
            ImVec2 min = ImGui::GetItemRectMin();
            ImVec2 max = ImGui::GetItemRectMax();

            ImU32 color = ImGui::GetColorU32(ImGuiCol_FrameBg);
            if (clicked)
                color = ImGui::GetColorU32(ImGuiCol_FrameBgActive);
            else if (hovered)
                color = ImGui::GetColorU32(ImGuiCol_FrameBgHovered);

            draw->AddRectFilled(min, max, color, 6.0f);

            // Move cursor inside button area
            ImGui::SetCursorScreenPos(ImVec2(min.x + 10, min.y + 10));

            ImGui::Text(s.name.c_str());
            std::string selected_text = "enabled";
            ImVec4 selected_color = ImVec4(0, 1, 0, 1);
            if (!s.selected)
            {
                selected_text = "disabled";
                selected_color = ImVec4(1, 0, 0, 1);
            }
            ImGui::TextColored(selected_color, selected_text.c_str());
            ImGui::Button("Inner Button");

            if (clicked)
            {
                // Handle big button click
            }

            //ImGui::Text(s.name.c_str());
            //ImGui::Checkbox("##checkbox", &s.selected);
            //ImGui::ImageButton();
            ImGui::EndGroup();
            ImGui::PopID();
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
