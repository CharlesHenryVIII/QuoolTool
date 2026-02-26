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

void RunCitectJob::RunJob()
{
    CitectData& cs = *m_citect_data;
    std::lock_guard<std::mutex> lock(cs.lock);

    //1. Backup project as Backup.ctz
    std::vector<ScannedFile> filenames;
    {
        ScanDirectoryForFileNames(cs.project_path, filenames, ScanDirectoryFlags(ScanDirectoryFlags_IncludeDirs | ScanDirectoryFlags_Recursive));
        g_data.total = filenames.size();
    }
    filenames.clear();
    ScanDirectoryForFileNames(cs.project_path, filenames, ScanDirectoryFlags_IncludeDirs);
     

    std::filesystem::path config_path = std::filesystem::path(cs.program_files_path) / L"Config";
    std::vector<ScannedFile> config_files;
    ScanDirectoryForFileNames(config_path.wstring(), config_files, ScanDirectoryFlags_None);

    std::vector<std::filesystem::path> ini_files;
    for (auto f : config_files)
    {
        if (f.name.find_last_of(L".ini") != std::wstring::npos)
        {
            ini_files.push_back(config_path / f.name);
        }
    }

    CreateZip(L"Backup.ctz", cs.backup_path, cs.project_path, CreateArrayView(filenames), CreateArrayView(ini_files));
    g_data.total = 0;
    g_data.progress = u64(-1);

    //2. Backup citect.ini file in config
    const Path program_files_path = cs.program_files_path;
    CopyFileRelative(program_files_path, cs.backup_path, Path(L"Config") / L"citect.ini");

    //3. Backup SE.Asb.Deployment.Server.WindowsService.exe.config
    CopyFileRelative(program_files_path, cs.backup_path, Path(L"Config") / L"SE.Asb.Deployment.Server.WindowsService.exe.config");

    //4. Backup SE.Asb.Deployment.Server.WindowsService.exe.config
    CopyFileRelative(program_files_path, cs.backup_path, Path(L"Config") / L"SE.Asb.Deployment.Node.WindowsService.exe.config");

    //5. Backup Deployment database
    CopyFolderRelative(program_files_path, cs.backup_path, Path(L"Deployment"));

    //6. Backup alarm database
    const Path project_name = Path(cs.project_path).filename();
    CopyFolderRelative(program_files_path, cs.backup_path, Path(L"Data") / project_name);

    //7. Backup Trend files
#if 1
    Path data_path = program_files_path / L"Data";
    std::vector<ScannedFile> data_files;
    ScanDirectoryForFileNames(data_path, data_files, ScanDirectoryFlags_None);
    g_data.total = data_files.size();
    g_data.progress = 0;
    for (auto sf : data_files)
    {
        ++g_data.progress;
        if (sf.dir)
            continue;

        const Path name = Path(sf.name);
        const std::string ext = name.extension().string();

        i32 value = 0;
        auto result = std::from_chars(ext.data() + 1, ext.data() + (ext.size() - 1), value);

        if (ext.find_last_of(".HST") ||
            result.ec == std::errc())
        {
            CopyFileRelative(program_files_path, cs.backup_path, Path(L"Data") / sf.name);
        }
    }
    g_data.total = 0;
    g_data.progress = u64(-1);
#else
    CopyFolderRelative(program_files_path, cs.backup_path, Path(L"Data"));
#endif

    //8.Report files
    //inside the project folder we don't need to reget them as we get them as part of the full project copy

    //9. Custom ActiveX Controls
    //TODO: THIS
    //Will need to find a custom ActiveX.dbf file to compare against and learn the internal structure
    CopyFile(Path(cs.project_path) / L"activex.DBF", cs.backup_path / L"activex.DBF");

    //10. Process Analyst Files
    //inside the project folder we don't need to reget them as we get them as part of the full project copy

    //11. Communication drivers
    //no idea where these are: "located in the product ‘bin’ directory as the existing specialty drivers may be required for the new version."
    Path bin_driver = L"Bin/DriverBackup";
    if (fs::exists(cs.program_files_86 / bin_driver))
        CopyFolderRelative(cs.program_files_86, cs.backup_path, bin_driver);

    //12. Device Logs
    CopyFolderRelative(program_files_path, cs.backup_path, L"Logs");
    
    //13. Additional files
    //not sure about these should supposedly be in the [PATH] section of the citect.ini (which one? lmao)

    //14. Citect.frm
    CopyFileRelative(cs.program_files_86, cs.backup_path, L"Bin/citect.frm");


    g_data.backup_in_progress = false;
}

bool GetScadaDir(std::filesystem::path& out, ArrayView<ScannedFile> files)
{
    for (auto file : files)
    {
        if (ContainsString(file.name, L"AVEVA Plant SCADA", StringCase_Insensitive) ||
            ContainsString(file.name, L"Citect SCADA", StringCase_Insensitive))
        {
            out = std::filesystem::path(file.name);
            return true;
        }
    }
    return false;
}

void CitectImGui(CitectData& cd)
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    Threading& threading = Threading::GetInstance();
    ImGuiWindowFlags sectionFlags =
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoMove;

    //const ImVec2 paths_scale = { 0, 0.2f };
    //const ImVec2 paths_size =  HadamardProduct(viewport->WorkSize, paths_scale);
    const ImVec2 paths_size = { 0, 200 };
    if (ImGui::BeginChild("File Paths", paths_size, true, sectionFlags))
    {
        ZoneScopedN("File Paths");
        TextCentered("File Paths");
        ImGui::NewLine();

        {
            bool locked = cd.lock.try_lock();
            Defer{ cd.lock.unlock(); };
            ImGui::BeginDisabled(!locked);
            ImGui::BeginGroup();
            if (ImguiPath("Backup Folder", "Please select removable media folder to backup to", cd.backup_path))
                WriteSettings(&g_data.settings, g_settings_filename);
            if (ImguiPath("Program Folder", "%PROGRAMDATA%/AVEVA/Citect SCADA 2018 R2", cd.program_files_path))
                WriteSettings(&g_data.settings, g_settings_filename);
            if (ImguiPath("Project Folder", "%PROGRAMDATA%/AVEVA Plant SCADA/User/<project>", cd.project_path))
                WriteSettings(&g_data.settings, g_settings_filename);
            if (ImguiPath("Program Files (x86)", "%PROGRAMDATA%/AVEVA/Citect SCADA", cd.program_files_86))
                WriteSettings(&g_data.settings, g_settings_filename);
            ImGui::EndGroup();

            ImVec2 size = ImGui::GetItemRectSize();
            ImGui::SameLine();
            if (ImGui::Button("Auto Search Paths", ImVec2(150, size.y)))
            {
                std::wstring program_data;
                ExpandEnvironemntVariable(program_data, L"%PROGRAMDATA%");
                const std::filesystem::path program_data_path = program_data;
                std::vector<ScannedFile> program_data_folders;
                ScanDirectoryForFileNames(program_data_path, program_data_folders, ScanDirectoryFlags_IncludeDirs);
                std::filesystem::path found_path;

                //1. search for final scada dir first in ProgramData
                bool found = GetScadaDir(found_path, CreateArrayView(program_data_folders));
                if (found)
                {
                    found_path = program_data_path / found_path;
                }

                //2. search ProgramData for AVEVA folders
                if (!found)
                {
                    for (const auto pd_folder : program_data_folders)
                    {
                        if (!pd_folder.dir)
                            continue;

                        if (ContainsString(pd_folder.name, L"AVEVA", StringCase_Insensitive) ||
                            ContainsString(pd_folder.name, L"Citect", StringCase_Insensitive))
                        {
                            const std::filesystem::path aveva_folder_path = (program_data_path / pd_folder.name).wstring();
                            std::vector<ScannedFile> aveva_folder_files;
                            ScanDirectoryForFileNames(aveva_folder_path, aveva_folder_files, ScanDirectoryFlags_IncludeDirs);
                            std::filesystem::path scada_folder;
                            found = GetScadaDir(scada_folder, CreateArrayView(aveva_folder_files));
                            if (found)
                            {
                                found_path = aveva_folder_path / scada_folder;
                                DebugPrint("aveva_folder_path: %s", aveva_folder_path.string().c_str());
                                DebugPrint("scada_folder: %s", scada_folder.string().c_str());
                                break;
                            }
                        }
                    }
                }
                if (found)
                {
                    cd.program_files_path = found_path.wstring();
                    cd.project_path = cd.program_files_path / L"User" / L"UNKNOWN";
                    WriteSettings(&g_data.settings, g_settings_filename);
                }
                else
                {
                    DebugPrint("Failed to find aveva directories");
                }

                //3. search ProgramData for AVEVA folders
                //C:\Program Files (x86)\AVEVA Plant SCADA\

                std::wstring program_files_86;
                ExpandEnvironemntVariable(program_files_86, L"%programfiles(x86)%");
                Path pfx = program_files_86;
            }

            ImGui::EndDisabled();
        }


    }
    ImGui::EndChild();

    #define TITLE_NAME "Citect / AVEVA"
    //const ImVec2 videos_scale = { 0, 0.76f };
    const ImVec2 videos_scale = { 0, 0 };
    const ImVec2 videos_size = HadamardProduct(viewport->WorkSize, videos_scale);
    if (ImGui::BeginChild(TITLE_NAME, videos_size, true, sectionFlags))
    {
        ZoneScopedN(TITLE_NAME);
        TextCentered(TITLE_NAME);
        ImGui::NewLine();

        std::error_code ec;
        ImGui::BeginDisabled(g_data.backup_in_progress ||
          !(fs::exists(cd.backup_path, ec)         &&
            fs::exists(cd.project_path, ec)        &&
            fs::exists(cd.program_files_path, ec)  &&
            fs::exists(cd.program_files_86, ec)));
        float height = 40;
        if (ImGui::Button("Backup Citect", ImVec2(125, height)) && !g_data.backup_in_progress)
        {
            RunCitectJob* job = new RunCitectJob();
            job->m_citect_data = &cd;
            g_data.backup_in_progress = true;
            Threading::GetInstance().SubmitJob(job);
        }
        ImGui::EndDisabled();

        if (g_data.backup_in_progress)
        {
            ImGui::SameLine();
            if (g_data.progress == u64(-1))
            {
                ImGui::ProgressBar(-1.0f * (float)ImGui::GetTime(), ImVec2(-FLT_MIN, height), "Backing up misc files...");
            }
            else
            {
                ImGui::ProgressBar(float(g_data.progress) / g_data.total, ImVec2(-FLT_MIN, height));
            }
        }
    }
    ImGui::EndChild();
}
