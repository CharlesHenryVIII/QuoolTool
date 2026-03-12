#pragma once
#include "Threading.h"
#include "imgui.h"
#include "Math.h"
#include "ArrayView.h"
#include "Settings.h"
#include "glfw/glfw3.h"

#include <string>

enum RunProcessFlags : u32 {
    RunProcess_None = 0,
    RunProcess_Async = BIT(0),
    RunProcess_Show = BIT(1),
};

struct SystemInfo {
    std::wstring name;
    i32 cores;
    i32 threads;
};
extern SystemInfo g_sysinfo;

void DebugPrint(const char* fmt, ...);
void DebugPrint(const wchar_t* fmt, ...);
std::string ToString(const char* fmt, ...);
std::wstring ToString(const wchar_t* fmt, ...);
i32 RunShellProcess(const wchar_t* path, const wchar_t* args, std::string* output = nullptr, Mutex* output_lock = nullptr, RunProcessFlags flags = RunProcess_None);
i32 RunProcess     (const wchar_t* path, const wchar_t* args, std::string* output = nullptr, Mutex* output_lock = nullptr, RunProcessFlags flags = RunProcess_None);
void OSInit(GLFWwindow* window);
int Main(int, char**);
bool ConsoleAttached();
bool DebuggerAttached();
void EnableOutputToDebugger();
void HideConsole();
void ShowConsole();
bool IsConsoleVisible();
void SysSleep(u64 ms);
double SysGetTime();

static bool keepOpen = true;
void ShowErrorWindow(const std::wstring& title, const std::wstring& text);
i32 ShowCustomErrorWindow(const std::string& title, const std::string& text);
void NotifyWindowBuildFinished();
enum ScanDirectoryFlags : u32 {
    ScanDirectoryFlags_None = 0,
    ScanDirectoryFlags_Recursive = BIT(0),
    ScanDirectoryFlags_IncludeDirs = BIT(1),
    ScanDirectoryFlags_All = ScanDirectoryFlags_Recursive | ScanDirectoryFlags_IncludeDirs,
};
struct ScannedFile {
    std::wstring name;
    bool dir;
};
void ScanDirectoryForFileNames(const std::wstring& dir, std::vector<ScannedFile>& out, ScanDirectoryFlags flags);
bool GetDirectoryFromUser(const std::wstring& currentDir, std::wstring& dir);
enum MessageBoxResponse : i32 {
    MessageBoxResponse_Invalid,
    MessageBoxResponse_OpenLog,
    MessageBoxResponse_Continue,
    MessageBoxResponse_Quit,
    MessageBoxResponse_Count,
};

void ConvertMultibyteToWideChar(std::wstring& out, const std::string& in);
void ConvertWideCharToMultiByte(std::string& out, const std::wstring& in);
void ExpandEnvironemntVariable(std::wstring& out, const std::wstring& in);
void ToLower(std::wstring& s);
void ToLower(std::string& s);
void CreateZip(const std::wstring& zip_name, const std::wstring& zip_pathw, const std::wstring& source_folder, ArrayView<ScannedFile> files_to_backup, ArrayView<std::filesystem::path> files_to_add_to_root, std::atomic<u64>& progress/*, ArrayView<std::wstring> ext_to_exclude*/);
bool UnzipArchive(const std::string& zip_path, const std::string& output_dir, std::vector<std::string>& filenames);
ImFont* LoadFontForImgui(int resource_id, float fontSize);

struct RunProcessJob : Job
{
    std::wstring application_path;
    std::wstring arguments;
    virtual void RunJob() override;
};

struct RunProcessLogToFileJob : Job
{
    std::wstring application_path;
    std::wstring arguments;
    std::wstring output_file;
    Atomic<bool>* completed;
    bool run_and_clear = false;
    virtual void RunJob() override;
};
