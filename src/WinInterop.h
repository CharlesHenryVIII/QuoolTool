#pragma once
#include "Threading.h"
#include "imgui.h"
#include "Math.h"
#include "ArrayView.h"
#include "VideoData.h"
#include "glfw/glfw3.h"

#include <string>

void DebugPrint(const char* fmt, ...);
void DebugPrint(const wchar_t* fmt, ...);
std::string ToString(const char* fmt, ...);
std::wstring ToString(const wchar_t* fmt, ...);
i32 RunProcess(const wchar_t* path, const wchar_t* args, bool async = false, bool show = true);
void InitOS(GLFWwindow* window);

static bool keepOpen = true;
void ShowErrorWindow(const std::wstring& title, const std::wstring& text);
i32 ShowCustomErrorWindow(const std::string& title, const std::string& text);
void NotifyWindowBuildFinished();
enum ScanDirectoryFlags : u32 {
    ScanDirectoryFlags_None = 0,
    ScanDirectoryFlags_Recursive = BIT(0),
    ScanDirectoryFlags_IncludeDirs = BIT(1),
    ScanDirectoryFlags_All,
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
int Main(int, char**);
void CreateZip(const std::wstring& zip_pathw, const std::wstring& source_folder, ArrayView<ScannedFile> files_to_backup);
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

struct RunUpdateVideoGroupJob : Job
{
    std::wstring mkv_path;
    std::wstring source_path;
    std::wstring dest_path;
    VideoGroup* video_group;
    virtual void RunJob() override;
};

struct RunEncodeJob : Job
{
    std::wstring mkv_path;
    std::wstring source_path;
    std::wstring dest_path;
    VideoGroup* video_group;
    virtual void RunJob() override;
};