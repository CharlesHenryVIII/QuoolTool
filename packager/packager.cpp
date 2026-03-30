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


int Main(int argc, char** argv)
{
    DebugPrint("=====================");
    DebugPrint(" Quool Tool Packager ");
    DebugPrint("=====================\n");

    const char* project_name = "QuoolTool";

    std::string sln = ToString("%s.sln", project_name);
    if (!fs::exists(sln))
    {
        sln = ToString("%s.slnx", project_name);
        std::error_code ec;
        if (!fs::exists(sln, ec))
        {
            FAIL;
            DebugPrint("Error: GenerateProjectFiles has not been ran");
            return 0;
        }
    }

    std::string build_command = ToString("msbuild /t:%s /nologo /verbosity:minimal -p:Configuration=Debug %s", project_name, sln.c_str());
                                        //msbuild /t:QuoolTool /nologo /verbosity:minimal -p:Configuration=Debug QuoolTool.sln
    std::string empty;
    std::string build_output;
    DebugPrint("=====================");
    DebugPrint("     Compiling:      ");
    RunProcess(empty, build_command, &build_output);
    DebugPrint("=====================");
    DebugPrint("   Build Output:");
    DebugPrint("%s", build_output.c_str());


    const Path build_dir = "build/";
    const Path exe = build_dir / ToString("%s_windows_x64_Release.exe", project_name);
    const Path renamed = build_dir / ToString("%s_%s.exe", project_name, g_version.AsTagString().c_str());
    const Path batch_script = "QuoolTools_OldWindows.bat";

    std::error_code ec;
    if (!fs::exists(exe, ec))
    {
        FAIL;
        DebugPrint("Error: GenerateProjectFiles has not been ran");
        return 0;
    }

    fs::copy_file(exe, renamed, ec);
    if (ec)
    {
        DebugPrint("Error: Failed to rename exe from(\"%s\") to(\"%s\")", exe.string().c_str(), renamed.string().c_str());
        return 0;
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
        return 0;
    }
    fs::remove(renamed, ec);

    DebugPrint("Successfully created zip: %s", zip_name.string().c_str());

    system("pause");
    return 1;
}
