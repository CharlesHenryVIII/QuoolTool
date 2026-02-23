#include "Citect.h"
#include "WinInterop.h"
#include "WinInterop_File.h"
#include "Math.h"
#include "Threading.h"
#include "Themes.h"
#include "Settings.h"
#include "LoadJson.h"
#include "ImguiHelper.h"
#include "String.h"

#include <filesystem>

#include "Tracy.hpp"
//#include "archive.h"
//#include "archive_entry.h"

void GetConfigFolder(std::wstring& out, const SettingsCitect& sc) { out = sc.program_files_path + L"/Config"; }
void GetDataFolder(std::wstring& out, const SettingsCitect& sc) { out = sc.program_files_path + L"/Data"; }
void GetLogFolder(std::wstring& out, const SettingsCitect& sc) { out = sc.program_files_path + L"/Logs"; }

void RunCitectJob::RunJob()
{
    std::vector<ScannedFile> filenames;
    {
        ScanDirectoryForFileNames(g_data.settings.citect.project_path, filenames, ScanDirectoryFlags(ScanDirectoryFlags_IncludeDirs | ScanDirectoryFlags_Recursive));
        g_data.total = filenames.size();
    }
    filenames.clear();
    ScanDirectoryForFileNames(g_data.settings.citect.project_path, filenames, ScanDirectoryFlags_IncludeDirs);
    CreateZip(g_data.settings.backup_path, g_data.settings.citect.project_path, CreateArrayView(filenames));
    g_data.backup_in_progress = false;
    g_data.total = 0;
    g_data.progress = 0;
}

void CitectZip()
{
    std::vector<ScannedFile> filenames;
    ScanDirectoryForFileNames(g_data.settings.citect.project_path, filenames, ScanDirectoryFlags_IncludeDirs);
    CreateZip(g_data.settings.backup_path, g_data.settings.citect.project_path, CreateArrayView(filenames));
}

void CitectImGui()
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

    const ImVec2 paths_scale = { 0, 0.2f };
    const ImVec2 paths_size =  HadamardProduct(viewport->WorkSize, paths_scale);
    if (ImGui::BeginChild("File Paths", paths_size, true, sectionFlags))
    {
        ZoneScopedN("File Paths");
        TextCentered("File Paths");
        ImGui::NewLine();


        ImGui::BeginGroup();
        std::wstring p = g_data.settings.backup_path;
        if (ImguiPath("Backup Folder", "Please select removable media folder to backup to", p, true))
        {
            g_data.settings.backup_path = p;
            WriteSettings(&g_data.settings, g_settings_filename);
        }
        if (ImguiPath("Program Folder", "%PROGRAMDATA%/AVEVA/Citect SCADA 2018 R2", citect_settings.program_files_path, true))
            WriteSettings(&g_data.settings, g_settings_filename);
        if (ImguiPath("Project Folder", "%PROGRAMDATA%/AVEVA Plant SCADA/User/<project>", citect_settings.project_path, true))
            WriteSettings(&g_data.settings, g_settings_filename);
        ImGui::EndGroup();

        ImVec2 size = ImGui::GetItemRectSize();
        ImGui::SameLine();
        if (ImGui::Button("Auto Search Paths", ImVec2(150, size.y)))
        {
            std::wstring program_data;
            ExpandEnvironemntVariable(program_data, L"%PROGRAMDATA%");
            std::filesystem::path path = program_data;
            std::vector<ScannedFile> folders;
            ScanDirectoryForFileNames(path, folders, ScanDirectoryFlags_IncludeDirs);
            std::wstring found;
            for (i32 i = 0; i < folders.size(); i++)
            {
                if (!folders[i].dir)
                {
                    FAIL;
                    continue;
                }
                std::string folder;
                ConvertWideCharToMultiByte(folder, folders[i].name);
                ToLower(folder);
                if (folder.contains("aveva"))
                {
                    found = folders[i].name;
                    break;
                }
                else if (folder.contains("citect"))
                {
                    found = folders[i].name;
                    break;
                }
            }
            if (found.size())
            {
                citect_settings.program_files_path = (std::filesystem::path(program_data) / found).wstring();
                citect_settings.project_path = (std::filesystem::path(citect_settings.program_files_path) / L"User").wstring();
                WriteSettings(&g_data.settings, g_settings_filename);
            }
        }


    }
    ImGui::EndChild();

    #define TITLE_NAME "Citect / AVEVA"
    ImVec2 videos_scale = { 0, 0.76f };
    ImVec2 videos_size = HadamardProduct(viewport->WorkSize, videos_scale);
    if (ImGui::BeginChild(TITLE_NAME, videos_size, true, sectionFlags))
    {
        ZoneScopedN(TITLE_NAME);
        TextCentered(TITLE_NAME);
        ImGui::NewLine();

        ImGui::BeginDisabled(g_data.backup_in_progress ||
            g_data.settings.backup_path.string().size() < 3 ||
            g_data.settings.citect.project_path.size() < 3 ||
            g_data.settings.citect.program_files_path.size() < 3);
        float height = 40;
        if (ImGui::Button("Create Zip", ImVec2(125, height)) && !g_data.backup_in_progress)
        {
            RunCitectJob* job = new RunCitectJob();
            job->settings = g_data.settings.citect;
            job->backup_path = g_data.settings.backup_path;
            g_data.backup_in_progress = true;
            Threading::GetInstance().SubmitJob(job);
        }
        ImGui::EndDisabled();
        if (g_data.backup_in_progress)
        {
            ImGui::SameLine();
            ImGui::ProgressBar(float(g_data.progress) / g_data.total, ImVec2(-FLT_MIN, height));
        }

        //if (g_data.video_group.thread_lock.try_lock())
        //{
        //    Defer{ g_data.video_group.thread_lock.unlock(); };

        //    //ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY))
        //    const ImGuiTableFlags table_flags = ImGuiTableFlags_SizingFixedFit |
        //        ImGuiTableFlags_NoSavedSettings |
        //        ImGuiTableFlags_RowBg |
        //        ImGuiTableFlags_Borders |
        //        ImGuiTableFlags_Resizable |
        //        ImGuiTableFlags_Reorderable |
        //        ImGuiTableFlags_ScrollX |
        //        ImGuiTableFlags_ScrollY |
        //        ImGuiTableFlags_Hideable;

        //    if (g_data.video_group.video_infos.size() && ImGui::BeginTable("Videos", g_data.video_group.max_tracks + 1, table_flags, ImVec2(videos_size.x, videos_size.y - 100)))
        //    {
        //        ImGui::TableSetupScrollFreeze(1, 0);
        //        ImGui::TableSetupColumn("Name");
        //        for (i32 i = 0; i < g_data.video_group.max_tracks; i++)
        //        {
        //            std::string name = ToString("Track %i", i);
        //            ImGui::TableSetupColumn(name.c_str());
        //        }
        //        //ImGui::TableHeadersRow();

        //        // Dummy entire-column selection storage
        //        // FIXME: It would be nice to actually demonstrate full-featured selection using those checkbox.
        //        static bool column_selected[3] = {};

        //        // Instead of calling TableHeadersRow() we'll submit custom headers ourselves.
        //        // (A different approach is also possible:
        //        //    - Specify ImGuiTableColumnFlags_NoHeaderLabel in some TableSetupColumn() call.
        //        //    - Call TableHeadersRow() normally. This will submit TableHeader() with no name.
        //        //    - Then call TableSetColumnIndex() to position yourself in the column and submit your stuff e.g. Checkbox().)
        //        ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
        //        for (i32 column = 0; column < g_data.video_group.max_tracks + 1; column++)
        //        {
        //            ImGui::TableSetColumnIndex(column);
        //            const char* column_name = ImGui::TableGetColumnName(column); // Retrieve name passed to TableSetupColumn()
        //            ImGui::PushID(column);
        //            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

        //            bool all_true = true;
        //            for (size_t i = 0; i < g_data.video_group.video_infos.size(); i++)
        //            {
        //                if (column == 0)
        //                {
        //                    if (!g_data.video_group.video_infos[i].encode)
        //                    {
        //                        all_true = false;
        //                        break;
        //                    }
        //                }
        //                else
        //                {
        //                    if ((size_t)column <= g_data.video_group.video_infos[i].tracks.size() &&
        //                        !g_data.video_group.video_infos[i].tracks[column - 1].encode)
        //                    {
        //                        all_true = false;
        //                        break;
        //                    }
        //                }
        //            }

        //            const char* bn = all_true ? "Disable All" : "Enable All";
        //            if (ImGui::Button(bn))
        //            {
        //                for (size_t i = 0; i < g_data.video_group.video_infos.size(); i++)
        //                {
        //                    if (column == 0)
        //                    {
        //                        g_data.video_group.video_infos[i].encode = !all_true;
        //                    }
        //                    else
        //                    {
        //                        if ((size_t)column <= g_data.video_group.video_infos[i].tracks.size())
        //                            g_data.video_group.video_infos[i].tracks[column - 1].encode = !all_true;
        //                    }
        //                }
        //            }
        //            //ImGui::Checkbox("##checkall", &column_selected[column]);
        //            ImGui::PopStyleVar();
        //            ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
        //            ImGui::TableHeader(column_name);
        //            ImGui::PopID();
        //        }

        //        for (size_t vi = 0; vi < g_data.video_group.video_infos.size(); vi++)
        //        {
        //            VideoInfo& info = g_data.video_group.video_infos[vi];
        //            std::string mb_name;
        //            ConvertWideCharToMultiByte(mb_name, info.name);
        //            ImGui::TableNextRow();
        //            ImGui::TableSetColumnIndex(0);
        //            ImGui::PushID(mb_name.c_str());
        //            ImGui::Checkbox("##", &info.encode);
        //            ImGui::SameLine();
        //            ImguiText(info.name);

        //            for (size_t i = 0; i < info.tracks.size(); i++)
        //            {
        //                ImGui::TableSetColumnIndex(int(1 + i));
        //                Track& track = info.tracks[i];

        //                //std::string id_string = ToString("##%s %i", mb_name.c_str(), );
        //                ImGui::PushID(track.id);
        //                ImGui::Checkbox("##", &track.encode);
        //                ImGui::SameLine();
        //                ImGui::Text("%s %s", track.type.c_str(), track.details.c_str());
        //                ImGui::PopID();
        //            }
        //            ImGui::PopID();
        //        }
        //        ImGui::EndTable();
        //    }

        //    float height = 40;
        //    //if (g_data.settings.source_path.size() && ImGui::Button("SCAN", ImVec2(125, height)))
        //    //{
        //    //    g_data.video_group.video_infos.clear();
        //    //    g_data.video_group.max_tracks = 0;
        //    //    RunUpdateVideoGroupJob* job = new RunUpdateVideoGroupJob();
        //    //    job->mkv_path = g_data.settings.mkv_path;
        //    //    job->source_path = g_data.settings.source_path;
        //    //    job->dest_path = g_data.settings.dest_path;
        //    //    job->video_group = &g_data.video_group;
        //    //    threading.SubmitJob(job);
        //    //}
        //    //if (g_data.video_group.video_infos.size())
        //    //{
        //    //    ImGui::SameLine();
        //    //    if (ImGui::Button("ENCODE", ImVec2(125, height)))
        //    //    {
        //    //        RunEncodeJob* job = new RunEncodeJob();
        //    //        job->mkv_path = g_data.settings.mkv_path;
        //    //        job->source_path = g_data.settings.source_path;
        //    //        job->dest_path = g_data.settings.dest_path;
        //    //        job->video_group = &g_data.video_group;
        //    //        threading.SubmitJob(job);
        //    //    }
        //    //}
        //    if (g_data.video_group.in_progress)
        //    {
        //        ImGui::SameLine();
        //        ImVec2 size = ImVec2(-FLT_MIN, height);
        //        if (!g_data.video_group.completed)
        //            ImGui::ProgressBar(-1.0f * (float)ImGui::GetTime(), size, "Encoding...");
        //        else
        //            ImGui::ProgressBar(float(g_data.video_group.completed) / g_data.video_group.video_infos.size(), size);
        //    }

        //}
        //else
        //{
        //    ImGui::ProgressBar(-1.0f * (float)ImGui::GetTime(), ImVec2(-FLT_MIN, 40), "SCANNING ...");
        //}
    }
    ImGui::EndChild();
}
