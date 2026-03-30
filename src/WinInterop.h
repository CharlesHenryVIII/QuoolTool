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
    Path drop_file;
};
extern SystemInfo g_sysinfo;

struct OSIPAndSubnet {
    std::string ip;
    std::string subnet;
};

struct OSNetworkAdapterInfo
{
    std::string name;
    std::string status;
    std::string mac_address;
    std::string ipv4_dhcp;
    std::string ipv6_dhcp;
    std::wstring friendly_name;
    std::wstring description;
    std::wstring dns_domain;
    std::vector<OSIPAndSubnet> ipv4_ips;
    std::vector<OSIPAndSubnet> ipv6_ips;
    std::vector<std::string> ipv4_dns;
    std::vector<std::string> ipv6_dns;
    std::vector<std::string> ipv4_gateways;
    std::vector<std::string> ipv6_gateways;

    u32 ipv4_metric;
    u32 ipv6_metric;
    bool ipv4_enabled;
    bool ipv6_enabled;
    bool dhcpv4_enabled;
    bool ddns_enabled;
    bool domain_dns_register_enabled;
    bool receive_only;
    bool multicast_enabled;
};

void DebugPrint(const char* fmt, ...);
void DebugPrint(const wchar_t* fmt, ...);
//No new line
void DebugPrintDirect(const char* fmt, ...);
std::string ToString(const char* fmt, ...);
std::wstring ToString(const wchar_t* fmt, ...);
i32 RunShellProcess(const wchar_t* path, const wchar_t* args, std::string* output = nullptr, Mutex* output_lock = nullptr, RunProcessFlags flags = RunProcess_None);
i32 RunProcess(const char*         path, const char*         args, AsyncData<std::string>* output = nullptr, AsyncData<Path>* output_file = nullptr, RunProcessFlags flags = RunProcess_None);
i32 RunProcess(const wchar_t*      path, const wchar_t*      args, AsyncData<std::string>* output = nullptr, AsyncData<Path>* output_file = nullptr, RunProcessFlags flags = RunProcess_None);
i32 RunProcess(const std::string&  path, const std::string&  args, AsyncData<std::string>* output = nullptr, AsyncData<Path>* output_file = nullptr, RunProcessFlags flags = RunProcess_None);
i32 RunProcess(const std::wstring& path, const std::wstring& args, AsyncData<std::string>* output = nullptr, AsyncData<Path>* output_file = nullptr, RunProcessFlags flags = RunProcess_None);
struct SDL_Window;
bool OSInit(SDL_Window* window);
void OSDestroy(SDL_Window* window);
void* OSGetWindowHandle(SDL_Window* window);
bool OSGetNetworkAdapters(std::vector<OSNetworkAdapterInfo>& out_adapters);
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
void ParseCSV(PowershellResponse& out, const std::string& in, bool using_quotes);

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
using ScannedFiles = std::vector<ScannedFile>;
void ScanDirectoryForFileNames(const std::wstring& dir, ScannedFiles& out, ScanDirectoryFlags flags);
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
void CreateZip(const Path& zip_path, const Path& source_folder, ArrayView<ScannedFile> files_to_backup, ArrayView<Path> files_to_add_to_root, Atomic<u64>& progress/*, ArrayView<std::wstring> ext_to_exclude*/);
bool UnzipArchive(const std::string& zip_path, const std::string& output_dir, std::vector<std::string>& filenames);
ImFont* LoadFontForImgui(int resource_id, float fontSize);
void* OsGetDataFromResource(i32* out_size, const i32 resource_id);

union Guid {
    uint64_t e[2];
    //struct {
    //    uint32_t data1;
    //    uint16_t data2;
    //    uint16_t data3;
    //    uint8_t  data4[8];
    //};
    struct {
        uint32_t a;
        uint32_t b;
        uint32_t c;
        uint32_t d;
    };
    std::string ToString() const;
};
[[nodiscard]] Guid GuidFromString(const char* s);
template<typename T> inline [[nodiscard]] T GuidFromString(const char* s) { Guid id = GuidFromString(s); return *(T*)&id; };
inline constexpr [[nodiscard]] bool operator==(const Guid& a, const Guid& b) { return (a.e[0] == b.e[0]) && (a.e[1] == b.e[1]); }
inline constexpr [[nodiscard]] bool operator<(const Guid& a, const Guid& b) { return (a.e[1] < b.e[1]) || ((a.e[1] == b.e[1]) && (a.e[0] < b.e[0])); }

Guid NewGuid();

#define STRONG_GUID_DEF(name)                                                                                     \
union name {                                                                                                      \
    uint64_t e[2];                                                                                                \
    struct {                                                                                                      \
        uint32_t a;                                                                                               \
        uint32_t b;                                                                                               \
        uint32_t c;                                                                                               \
        uint32_t d;                                                                                               \
    };                                                                                                            \
    std::string ToString() const { Guid guid = *(Guid*)this; return guid.ToString(); };                           \
    void FromString(const char* s) { *(Guid*)this = GuidFromString(s); };                                         \
    void New() { *(Guid*)this = NewGuid(); };                                                                     \
    static inline    [[nodiscard]] name GetNew() { name r; *(Guid*)&r = NewGuid(); return r; };                   \
    inline constexpr [[nodiscard]] bool IsValid() const { return e[0] && e[1]; };                                 \
};                                                                                                                \
inline constexpr [[nodiscard]] bool operator==(const name& a, const name& b) { return *(Guid*)&a == *(Guid*)&b; };\
inline constexpr [[nodiscard]] bool operator< (const name& a, const name& b) { return *(Guid*)&a <  *(Guid*)&b; };\
enum {}

struct RunProcessJob : Job
{
    std::wstring path;
    std::wstring args;
    AsyncData<std::string>* output;
    virtual void RunJob() override;
};

struct RunProcessLogToFileJob : Job
{
    std::wstring path;
    std::wstring args;
    AsyncData<std::string>* output;
    AsyncData<Path> output_file;
    Atomic<bool>* completed;
    bool run_and_clear = false;
    virtual void RunJob() override;
};
