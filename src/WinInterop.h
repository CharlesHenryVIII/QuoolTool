#pragma once
#include "Threading.h"
#include "imgui.h"
#include "Math.h"
#include "ArrayView.h"
#include "Settings.h"
#include "glfw/glfw3.h"

#include <string>

void DebugPrint(const char* fmt, ...);
void DebugPrint(const wchar_t* fmt, ...);
std::string ToString(const char* fmt, ...);
std::wstring ToString(const wchar_t* fmt, ...);
i32 RunProcess(const wchar_t* path, const wchar_t* args, bool async = false, bool show = true);
void InitOS(GLFWwindow* window);
int Main(int, char**);
bool ConsoleAttached();
bool DebuggerAttached();
void EnableOutputToDebugger();
void HideConsole();
void ShowConsole();
bool IsConsoleVisible();
void Sleep(u64 ms);

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
void CreateZip(const std::wstring& zip_name, const std::wstring& zip_pathw, const std::wstring& source_folder, ArrayView<ScannedFile> files_to_backup, ArrayView<std::filesystem::path> files_to_add_to_root/*, ArrayView<std::wstring> ext_to_exclude*/);
ImFont* LoadFontForImgui(int resource_id, float fontSize);

struct RunProcessJob : Job
{
    std::wstring applicationPath;
    std::wstring arguments;
    virtual void RunJob() override;
};

struct RunProcessLogJob : Job
{
    std::wstring applicationPath;
    std::wstring arguments;
    std::string output;
    virtual void RunJob() override;
};

struct RunEncodeJob : Job
{
    std::wstring mkv_path;
    std::wstring source_path;
    std::wstring dest_path;
    virtual void RunJob() override;
};