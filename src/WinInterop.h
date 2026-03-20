#pragma once
#include "Threading.h"
#include "imgui.h"
#include "Math.h"
#include "ArrayView.h"
#include "Settings.h"

#include <string>
#include <unordered_map>

#define PWSH_MAX_COLUMNS 16
typedef std::vector<std::array<std::string, PWSH_MAX_COLUMNS>> PowershellResponse;

enum RunProcessFlags : u32 {
    RunProcess_None = 0,
    RunProcess_Async = BIT(0),
    RunProcess_Show = BIT(1),
};

struct Key {
    bool down;
    bool downPrevFrame;
    bool downThisFrame;
    bool upThisFrame;
};

struct Mouse {
    Vec2 p = {}; //origin is the bottom left of the window
    Vec2 delta_p = {};
    Vec2 wheel = {}; //Y for vertical rotations, X for Horizontal rotations/movement
    Vec2 wheel_instant = {};
    bool wheel_modified_last_frame = false;
    //SDL_Cursor* cursors[ImGuiMouseCursor_COUNT] = {};
};

struct SystemInfo {
    std::wstring name;
    i32 cores;
    i32 threads;
    std::unordered_map<u32, Key> keys;
    Mouse mouse = {};
    bool has_attention;
    bool drop_active = false;
    bool drop_complete = false;
    Path drop_file;
};
extern SystemInfo g_sysinfo;

void DebugPrint(const char* fmt, ...);
void DebugPrint(const wchar_t* fmt, ...);
std::string ToString(const char* fmt, ...);
std::wstring ToString(const wchar_t* fmt, ...);
i32 RunShellProcess(const wchar_t* path, const wchar_t* args, std::string* output = nullptr, Mutex* output_lock = nullptr, RunProcessFlags flags = RunProcess_None);
i32 RunProcess(const std::wstring& path, const std::wstring& args, std::string* output = nullptr, AsyncData<Path>* output_file = nullptr, RunProcessFlags flags = RunProcess_None);
struct SDL_Window;
bool OSInit(SDL_Window* window);
void OSDestroy(SDL_Window* window);
void* OSGetWindowHandle(SDL_Window* window);
int Main(int, char**);
bool ConsoleAttached();
bool DebuggerAttached();
//void EnableOutputToDebugger();
void HideConsole();
void ShowConsole();
bool IsConsoleVisible();
void SysProcessEvents();
void SysSleep(u64 ms);
double SysGetTime();
float SysMonitorScale();

void ParsePowershell(PowershellResponse& out, const std::string& in);
void ParseSysinfo(PowershellResponse& out, const std::string& in);
void ParseCSV(PowershellResponse& out, const std::string& in);

static bool keepOpen = true;
void ShowErrorWindow(const std::wstring& title, const std::wstring& text);
i32 ShowCustomErrorWindow(const std::string& title, const std::string& text);
void SysFlashWindow(SDL_Window* window);
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
void* OsGetDataFromResource(i32* out_size, const i32 resource_id);

struct RunProcessJob : Job
{
    std::wstring path;
    std::wstring args;
    virtual void RunJob() override;
};

struct RunProcessLogToFileJob : Job
{
    std::wstring path;
    std::wstring args;
    std::string output;
    AsyncData<Path> output_file;
    Atomic<bool>* completed;
    bool run_and_clear = false;
    virtual void RunJob() override;
};
