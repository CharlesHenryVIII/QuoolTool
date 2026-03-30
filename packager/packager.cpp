#include "../src/Math.h"
#include "../src/WinInterop.h"
#include "../src/Version.h"

#include <cmath>
#include <cstdlib>
#include <stdio.h>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>

#define RETURN_FAILURE \
FAIL;\
return 0

int Main(int argc, char** argv)
{
    DebugPrint("=====================");
    DebugPrint(" Quool Tool Packager ");
    DebugPrint("=====================\n");

    Threading& threading = Threading::GetInstance();
    const char* project_name = "QuoolTool";
    std::string sln = ToString("%s.sln", project_name);
    if (!fs::exists(sln))
    {
        sln = ToString("%s.slnx", project_name);
        std::error_code ec;
        if (!fs::exists(sln, ec))
        {
            DebugPrint("Error: GenerateProjectFiles has not been ran");
            RETURN_FAILURE;
        }
    }

    std::string build_command = ToString("msbuild /t:%s /nologo /verbosity:minimal -p:Configuration=Release %s", project_name, sln.c_str());
    std::wstring build_commandw;
    ConvertMultibyteToWideChar(build_commandw, build_command);
    std::string empty;
    DebugPrint("=====================");
    DebugPrint("     Compiling:      ");
#if 1
    RunProcessJob* job = new RunProcessJob();
    AsyncData<std::string> build_log;
    Atomic<AsyncStatus> build_status;
    job->args = build_commandw;
    job->output = &build_log;
    job->status = &build_status;
    threading.SubmitJob(job);
#else
    i32 compile_result = RunProcess(empty, build_command, &build_output);
#endif
    DebugPrint("=====================");
    DebugPrint("   Build Output:");
    size_t build_log_written = 0;

    while (!FlagIntersects(build_status, AsyncStatus_Completed))
    {
        TRACY_LOCK(build_log.lock);
        if (build_log.data.size() > build_log_written)
        {
            DebugPrintDirect("%s", build_log.data.substr(build_log_written).c_str());
            build_log_written = build_log.data.size();
        }
        SysSleep(200);
    }

    const Path build_dir = "build/";
    const Path exe = build_dir / ToString("%s_windows_x64_Release.exe", project_name);
    const Path renamed = build_dir / ToString("%s_%s.exe", project_name, g_version.AsTagString().c_str());
    const Path batch_script = "QuoolTools_OldWindows.bat";

    std::error_code ec;
    if (!fs::exists(exe, ec))
    {
        DebugPrint("Error: Executable could not be found");
        DebugPrint("    -> %s", ec.message().c_str());
        RETURN_FAILURE;
    }

    fs::copy_file(exe, renamed, fs::copy_options::overwrite_existing, ec);
    if (ec)
    {
        DebugPrint("Error: Failed to rename exe from(\"%s\") to(\"%s\")", exe.string().c_str(), renamed.string().c_str());
        DebugPrint("    \"%s\"", ec.message().c_str());
        RETURN_FAILURE;
    }

    std::vector<ScannedFile> files_and_folders_to_zip;
    Path zip_name = ToString("%s_%s.zip", project_name, g_version.AsTagString().c_str());
    Path path_to_root;
    Path files_to_add_to_root[] = { renamed, batch_script };
    Atomic<u64> progress;
    CreateZip(zip_name, path_to_root, CreateArrayView(files_and_folders_to_zip), CreateArrayView(files_to_add_to_root), progress);
    if (!fs::exists(zip_name, ec))
    {
        DebugPrint("Erorr: Failed to create zip/zip doesn't exist");
        RETURN_FAILURE;
    }
    fs::remove(renamed, ec);

    DebugPrint("Successfully created zip: %s", zip_name.string().c_str());

    system("pause");
    return 1;
}
