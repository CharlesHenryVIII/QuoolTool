#include "Tools.h"
#include "String.h"
#include "ImguiHelper.h"
#include "Tracy.hpp"
#include "LoadJson.h"
#include "WinInterop.h"

#include "xlsxwriter.h"

AsyncData<lxw_workbook*> s_workbook;

struct ScriptData {
    std::string output;
    AsyncData<lxw_workbook*>* workbook;
};

typedef void (*ScriptFunction)(ScriptData&);
struct ScriptJob : Job
{
    std::wstring path;
    std::wstring args;
    std::wstring output_file;
    Atomic<bool>* completed;
    ScriptFunction func;
    ScriptData data;
    
    void RunJob() override
    {
        ZoneScopedN("ScriptJob");
        bool r = RunProcessAndLogToFile(data.output, path, args, output_file);
        if (func)
        {
            ZoneScopedN("ScriptJob func");
            func(data);
        }

        if (completed)
        {
            ASSERT(*completed == false);
            (*completed) = true;
        }
    }
};

struct WorkbookJob : Job
{
    AsyncData<lxw_workbook*>* workbook;
    Atomic<ScriptState>* state;

    void RunJob() override
    {
        ZoneScopedN("Workbook Job");
        if (workbook)
        {
            TRACY_LOCK(workbook->lock);
            workbook_close(workbook->data);
        }
        else
            FAIL;
        if (state)
        {
            *state = ScriptState_Finished;
        }
        else
            FAIL;
    }
};


struct ScriptInfo {
    std::string name;
    Atomic<ScriptInfoFlags> flags = ScriptInfoFlags(ScriptInfoFlags_Enabled);
    ScriptFunction func = nullptr;
    std::wstring cmdline; //function
    //break
    Atomic<bool> completed = false;
};

lxw_format* CreateTitleFormat(lxw_workbook* book)
{
    lxw_format* format = workbook_add_format(book);
    format_set_bold(format);
    format_set_align(format, LXW_ALIGN_CENTER);
    format_set_align(format, LXW_ALIGN_VERTICAL_CENTER);
    format_set_border(format, LXW_BORDER_THICK);
    //format_set_font_name(format, "Aptos Narrow");
    return format;
}

void ExcelWriteTitles(lxw_workbook* book, lxw_worksheet* sheet, size_t column_widths[PWSH_MAX_COLUMNS], const PowershellResponse& array)
{
    ASSERT(PWSH_MAX_COLUMNS == array[0].size());
    lxw_format* title_format = CreateTitleFormat(book);
    worksheet_set_row(sheet, 0, 30, NULL);
    for (i32 i = 0; i < array[0].size(); i++)
    {
        const auto& title = array[0][i];
        if (title.empty())
            continue;
        worksheet_write_string(sheet, 0, i, title.c_str(), title_format);
        column_widths[i] = Max(column_widths[i], title.size() + 4);
    }
}

void ExcelWriteData(lxw_workbook* book, lxw_worksheet* sheet, size_t column_widths[PWSH_MAX_COLUMNS], const PowershellResponse& array)
{
    lxw_format* format = workbook_add_format(book);
    format_set_align(format, LXW_ALIGN_LEFT);
    for (i32 row = 1; row < array.size(); row++)
    {
        for (i32 col = 0; col < array[row].size(); col++)
        {
            const auto& title = array[row][col];
            if (title.empty())
                continue;
            worksheet_write_string(sheet, row, col, title.c_str(), format);
            column_widths[col] = Max(column_widths[col], title.size());
        }
    }
}

void ExcelAutoSizeColumnWidth(lxw_worksheet* sheet, size_t column_widths[16])
{
    for (i32 i = 0; i < PWSH_MAX_COLUMNS; i++)
    {
        if (column_widths[i] <= 0)
            continue;
        double width = (double)column_widths[i];// / 10.0;
        worksheet_set_column(sheet, i, i, width, NULL);
    }
}

void ExcelWritePowershellData(lxw_workbook* book, lxw_worksheet* sheet, const PowershellResponse& array)
{
    size_t column_widths[16] = {};
    ASSERT(arrsize(column_widths) == array[0].size());
    ExcelWriteTitles(book, sheet, column_widths, array);
    ExcelWriteData(book, sheet, column_widths, array);
    ExcelAutoSizeColumnWidth(sheet, column_widths);
}

void ScriptPrograms(ScriptData& data)
{
    PowershellResponse array;
    ParsePowershell(array, data.output);
    switch (array.size())
    {
    case 0: return;
    case 1: FAIL; return; //only title?  no title?
    }
    TRACY_LOCK(data.workbook->lock);
    lxw_worksheet* sheet = workbook_add_worksheet(data.workbook->data, "Programs");
    ExcelWritePowershellData(data.workbook->data, sheet, array);
}

void ScriptProcessor(ScriptData& data)
{
    PowershellResponse array;
    ParsePowershell(array, data.output);
    switch (array.size())
    {
    case 0: return;
    case 1: FAIL; return; //only title?  no title?
    }
    TRACY_LOCK(data.workbook->lock);
    lxw_worksheet* sheet = workbook_add_worksheet(data.workbook->data, "Processor");
    ExcelWritePowershellData(data.workbook->data, sheet, array);
}

void ScriptSysinfo(ScriptData& data)
{
    PowershellResponse array;
    ParseSysinfo(array, data.output);
    if (!array.size())
    {
        FAIL;
        return;
    }
    TRACY_LOCK(data.workbook->lock);
    lxw_workbook* book = data.workbook->data;
    lxw_worksheet* sheet = workbook_add_worksheet(book, "Sysinfo");

    size_t column_widths[16] = {};
    ExcelWriteTitles(book, sheet, column_widths, array);
    ExcelWriteData(book, sheet, column_widths, array);
    ExcelAutoSizeColumnWidth(sheet, column_widths);
}

ScriptInfo s_scripts[] = {
    { .name = "SYSINFO",    .func = ScriptSysinfo,      .cmdline = L"systeminfo",},
    { .name = "NETSTAT",    .cmdline = L"netstat -ano" },
    { .name = "IPCONFIG",   .cmdline = L"ipconfig" },
    { .name = "PROGRAMS",   .func = ScriptPrograms,     .cmdline = L"powershell -command \"Get-ItemProperty 'HKLM:/Software/Microsoft/Windows/CurrentVersion/Uninstall/*' | Where {$_.DisplayName} | Select DisplayName,DisplayVersion\""},
    { .name = "PROCESSOR",  .func = ScriptProcessor,    .cmdline = L"powershell -command \"Get-CimInstance Win32_Processor | Select-Object Name, NumberOfCores, NumberOfLogicalProcessors, MaxClockSpeed\""},
};

std::string s_log;

void GetOutputFolder(Path& out, const ToolsData& td)
{
    out.clear();
    if (fs::exists(td.output_path))
        out = Path(td.output_path);
    out += g_sysinfo.name;
}

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

    //State
    i32 enabled_scripts = 0;
    i32 completed_scripts = 0;
    for (i32 i = 0; i < arrsize(s_scripts); i++)
    {
        ScriptInfo& s = s_scripts[i];
        if (!FlagExists(s.flags, ScriptInfoFlags_Enabled))
            continue;
        enabled_scripts++;
        if (s_scripts[i].completed)
            completed_scripts++;
    }
    if (td.state == ScriptState_Scripts && enabled_scripts == completed_scripts)
    {
        td.state = ScriptState_Workbook;
        WorkbookJob* job = new WorkbookJob();
        job->workbook = &s_workbook;
        job->state = &td.state;
        threading.SubmitJob(job);
    }
    const bool scripts_running = td.state == ScriptState_Scripts;

    
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
        ImGui::BeginDisabled(scripts_running);
        float height = 40;
        const ImVec2 button_size(125.0f, 60.0f);
        ImGui::SetCursorPosX(Max((ImGui::GetContentRegionAvail().x - (arrsize(s_scripts) * button_size.x)) * 0.5f, ImGui::GetStyle().ItemSpacing.x));
        const float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
        for (i32 i = 0; i < arrsize(s_scripts); i++)
        {
            ScriptInfo& s = s_scripts[i];
            ImGui::PushID(i);
            ImGui::BeginDisabled(s.completed);
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
            if (s.completed)
            {
                selected_text = "Completed";
                selected_color = IM_COL32(0, 255, 0, 255);
            }
            else if (!FlagExists(s.flags, ScriptInfoFlags_Enabled))
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
            ImGui::EndDisabled();
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

        ImGui::BeginDisabled(scripts_running || (!td.output_path.empty() && !fs::exists(td.output_path)));
        const ImVec2 avail = ImGui::GetContentRegionAvail();
        if (ImGui::Button("Run Scripts", avail))
        {
            ZoneScopedN("Run Scripts");
            ImguiLog("Running Scripts:");
            td.state = ScriptState_Scripts;

            Path output_folder;
            GetOutputFolder(output_folder, td);
            const Path excel_file = output_folder / "SystemInfo.xlsx";
            {
                TRACY_LOCK(s_workbook.lock);
                s_workbook.data = workbook_new(excel_file.string().c_str());
            }
            for (i32 i = 0; i < arrsize(s_scripts); i++)
            {
                ScriptInfo& s = s_scripts[i];
                if (!FlagExists(s.flags, ScriptInfoFlags_Enabled) || s.completed)
                    continue;

                ZoneScopedN("Run Script");
                ScriptJob* job = new ScriptJob();
                job->path;
                job->args = s.cmdline;
                const std::string name = s.name + ".txt";
                const Path output_file = output_folder / name;
                CreateParentDirectories(output_file);
                job->output_file = output_file;
                job->func = s.func;
                job->data.workbook = &s_workbook;
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

        float progress_bar_height = 50;
        switch (td.state)
        {
        case ScriptState_Scripts:
        {
            const ImVec2 ip_scale = { 0, 0.75 };
            ImVec2 ip_size = HadamardProduct(ImGui::GetContentRegionAvail(), ip_scale);
            if (ImGui::BeginChild("IndividualProgress", ip_size, ImGuiChildFlags_Borders, section_flags))
            {
                const float individual_height = 30.0f;
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

            ImVec2 max = ImGui::GetWindowContentRegionMax();
            ImGui::SetCursorPosY(max.y - progress_bar_height);

            if (td.state != ScriptState_Scripts)
                ImGui::ProgressBar(1.0f, ImVec2(-FLT_MIN, progress_bar_height), "Completed");
            else
            {
                std::string title = ToString("%i/%i", completed_scripts, enabled_scripts);
                ImGui::ProgressBar(float(completed_scripts) / float(enabled_scripts), ImVec2(-FLT_MIN, progress_bar_height), title.c_str());
            }
            break;
        }
        case ScriptState_Workbook:
        {
            ImGui::ProgressBar(-1.0f * (float)ImGui::GetTime(), ImVec2(-FLT_MIN, progress_bar_height), "Saving Excel File");
            break;
        }
        case ScriptState_Finished:
        {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Finished");
            break;
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
