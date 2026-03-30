#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>
#include <combaseapi.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#include "WinInterop.h"
#include "WinInterop_File.h"
#include "Math.h"
#include "String.h"
#include "resource.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "Json.hpp"
#include "Rendering.h"
#include "Tracy.hpp"

#include "SDL3/SDL.h"
//#include "SDL3/SDL_events.h"
#include "ImguiHelper.h"
#include "ImGui/backends/imgui_impl_sdl3.h"
#include "ImGui/backends/imgui_impl_sdlrenderer3.h"

#include <fstream>
#include <filesystem>
#include <cwctype>
#include <format>
#include <fstream>
#include <iostream>
#include <chrono>
#include <charconv>

#include "libarchive/libarchive/archive.h"
#include "libarchive/libarchive/archive_entry.h"

SystemInfo g_sysinfo;

void WriteToAttachedConsole(const char* buffer, bool add_new_line)
{
    //If we have a console, print there too
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    if (console != NULL && console != INVALID_HANDLE_VALUE)
    {
        DWORD mode;
        if (GetConsoleMode(console, &mode)) // succeeds only if console attached
        {
            DWORD written;
            WriteConsoleA(console, buffer, (DWORD)strlen(buffer), &written, NULL);
            if (add_new_line)
            {
                const char* new_line = "\n";
                WriteConsoleA(console, new_line, (DWORD)strlen(new_line), &written, NULL);
            }
        }
    }
}

void WriteToAttachedConsole(const wchar_t* buffer, bool add_new_line)
{
    //If we have a console, print there too
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    if (console != NULL && console != INVALID_HANDLE_VALUE)
    {
        DWORD mode;
        if (GetConsoleMode(console, &mode)) // succeeds only if console attached
        {
            DWORD written;
            WriteConsoleW(console, buffer, (DWORD)wcslen(buffer), &written, NULL);
            if (add_new_line)
            {
                const char* new_line = "\n";
                WriteConsoleA(console, new_line, (DWORD)strlen(new_line), &written, NULL);
            }
        }
    }
}

void DebugPrintDirect(const char* fmt, ...)
{
    va_list list;
    va_start(list, fmt);
    char buffer[4096];
    vsnprintf(buffer, sizeof(buffer), fmt, list);
    OutputDebugStringA(buffer);
    OutputDebugStringA("\n");
    va_end(list);

    WriteToAttachedConsole(buffer, false);
}

void DebugPrint(const char* fmt, ...)
{
    va_list list;
    va_start(list, fmt);
    char buffer[4096] = {};
    vsnprintf_s(buffer, arrsize(buffer), _TRUNCATE, fmt, list);
    OutputDebugStringA(buffer);
    OutputDebugStringA("\n");
    va_end(list);

    WriteToAttachedConsole(buffer, true);
}
void DebugPrint(const wchar_t* fmt, ...)
{
    va_list list;
    va_start(list, fmt);
    wchar_t buffer[4096] = {};
    _vsnwprintf_s(buffer, arrsize(buffer), _TRUNCATE, fmt, list);
    OutputDebugStringW(buffer);
    va_end(list);

    WriteToAttachedConsole(buffer, true);
}

std::string ToString(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char buffer[4096];
    i32 i = vsnprintf(buffer, arrsize(buffer), fmt, args);
    va_end(args);
    return buffer;
}

std::wstring ToString(const wchar_t* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    wchar_t buffer[4096];
    i32 i = vswprintf(buffer, arrsize(buffer), fmt, args);
    va_end(args);
    return buffer;
}

i32 RunShellProcess(const wchar_t* path, const wchar_t* args, std::string* output, Mutex* output_lock, RunProcessFlags flags)
{
    //TODO: Allow this to work for ASCII AND Unicode
    SHELLEXECUTEINFO info = {};
    info.cbSize = sizeof(SHELLEXECUTEINFO);
    info.fMask = SEE_MASK_NOASYNC | SEE_MASK_NOCLOSEPROCESS;
    info.hwnd;
    //info.lpVerb = "open";
    info.lpVerb = NULL;
    info.lpFile = path ? path : L"cmd.exe";
    info.lpParameters = args;
    info.lpDirectory = NULL;
    info.nShow = flags & RunProcess_Show ? SW_SHOW : SW_HIDE;
    info.hInstApp = NULL; //out
    info.lpIDList;
    info.lpClass;
    info.hkeyClass;
    info.dwHotKey;
    info.hProcess; //out

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    Defer { CloseHandle(info.hProcess); };
    if (!ShellExecuteEx(&info))
    {
        std::wstring errorBoxTitle = ToString(L"ShellExecuteEx Error: %i", GetLastError());
        std::wstring errorText     = ToString(L"Application Path: %s\n"
                                             "Command Line Params: %s", info.lpFile, args);
        ShowErrorWindow(errorBoxTitle, errorText);
        ASSERT(false);
        return 2;
    }
    if (!(flags & RunProcess_Async))
    {
        DWORD result = WaitForSingleObject(info.hProcess, INFINITE);
        if (result)
        {
            std::wstring errorBoxTitle = ToString(L"WaitForSingleObject Error: %i", GetLastError());
            std::wstring errorText = ToString(L"Application Path: %s\n"
                "Command Line Params: %s", info.lpFile, args);
            ShowErrorWindow(errorBoxTitle, errorText);
            ASSERT(false);
            return -1;
        }
        DWORD exitCode = {};
        if (!GetExitCodeProcess(info.hProcess, &exitCode))
        {
            std::wstring errorBoxTitle = ToString(L"GetExitCodeProcess Error: %i", GetLastError());
            std::wstring errorText = ToString(L"Application Path: %s\n"
                "Command Line Params: %s", info.lpFile, args);
            ShowErrorWindow(errorBoxTitle, errorText);
            return -1;
        }
        if (exitCode)
        {
            std::wstring werrorBoxTitle = ToString(L"Program Exited with Code: %i", exitCode);
            std::wstring werrorText = ToString(L"Application Path: %s\n"
                L"Command Line Params: %s", info.lpFile, args);
            std::string error_box_title;
            std::string error_text;
            ConvertWideCharToMultiByte(error_box_title, werrorBoxTitle);
            ConvertWideCharToMultiByte(error_text, werrorText);
            return ShowCustomErrorWindow(error_box_title, error_text);
        }
    }
    return 0;
}

void GetNameAndTextForJob(std::string& text, std::string& name, const std::wstring& app, const std::wstring& args)
{
    std::wstring namew;
    std::wstring textw;
    if (!app.size())
    {
       size_t p = args.find_first_of(L' ', 1);
       namew = args.substr(0, p);
       textw = args;
    }
    else
    {
        namew = app;
        if (args.size())
            textw = app + L" " + args;
    }
    if (namew.size())
        ConvertWideCharToMultiByte(name, namew);
    else
        name.clear();
    if (textw.size())
        ConvertWideCharToMultiByte(text, textw);
    else
        text.clear();
}

i32 RunProcess(const char* path, const char* args, AsyncData<std::string>* output, AsyncData<Path>* output_file, RunProcessFlags flags)
{
    const std::string p = path ? path : "";
    const std::string a = args ? args : "";
    return RunProcess(p, a, output, output_file, flags);
}
i32 RunProcess(const wchar_t* path, const wchar_t* args, AsyncData<std::string>* output, AsyncData<Path>* output_file, RunProcessFlags flags)
{
    const std::wstring pathw = path ? path : L"";
    const std::wstring argsw = args ? args : L"";
    return RunProcess(pathw, argsw, output, output_file, flags);
}
i32 RunProcess(const std::string& path, const std::string& args, AsyncData<std::string>* output, AsyncData<Path>* output_file, RunProcessFlags flags)
{
    std::wstring wpath;
    std::wstring wargs;
    ConvertMultibyteToWideChar(wpath, path);
    ConvertMultibyteToWideChar(wargs, args);
    return RunProcess(wpath, wargs, output, output_file, flags);
}

i32 RunProcess(const std::wstring& path, const std::wstring& args, AsyncData<std::string>* output, AsyncData<Path>* output_file, RunProcessFlags flags)
{
    std::string zone_text;
    std::string zone_name;
    GetNameAndTextForJob(zone_text, zone_name, path, args);
    ZoneScoped;
    ZoneName(zone_name.c_str(), zone_name.size());
    ZoneText(zone_text.c_str(), zone_text.size());

    SECURITY_ATTRIBUTES sa = {
        .nLength = sizeof(sa),
        .bInheritHandle = TRUE,
    };

    HANDLE readPipe = NULL;
    HANDLE writePipe = NULL;
    CreatePipe(&readPipe, &writePipe, &sa, 0);
    SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si{
        .cb = sizeof(si),
        .dwFlags = STARTF_USESTDHANDLES,
        .hStdInput = GetStdHandle(STD_INPUT_HANDLE),
        .hStdOutput = writePipe,
        .hStdError = writePipe,
    };

    std::wstring real_path;
    if (path.size() > 1)
        real_path = path;
    else
        real_path = L"cmd.exe /C";

    std::wstring cmdline = real_path;
    if (args.size())
        cmdline += real_path + L" " + args;

    PROCESS_INFORMATION pi = {};
    BOOL r = CreateProcessW(
        nullptr,
        (LPWSTR)cmdline.c_str(),
        nullptr, nullptr,
        TRUE, // inherit handles
        (flags & RunProcess_Show) ? CREATE_NEW_CONSOLE : CREATE_NO_WINDOW,
        nullptr, nullptr,
        &si, &pi
    );

    if (r == 0)
    {
        std::wstring errorBoxTitle = ToString(L"CreateProcess() Error: %i", GetLastError());
        std::wstring errorText     = ToString(L"Application Path: %s\n"
                                              L"Command Line Params: %s", real_path.c_str(), args.c_str());
        DebugPrint("%s\n", errorBoxTitle.c_str());
        DebugPrint(errorText.c_str());
        DebugPrint("\n");
        ShowErrorWindow(errorBoxTitle, errorText);
        FAIL;
        return 2;
    }

    CloseHandle(writePipe); // parent reads only

    if (output)
    {
        char buffer[4096];
        DWORD bytesRead;
        while (ReadFile(readPipe, buffer, sizeof(buffer), &bytesRead, nullptr))
        {
            TRACY_LOCK(output->lock);
            output->data.append(buffer, bytesRead);
        }
    }
    if (output_file)
    {
        if (output_file->data.empty())
        {
            std::string p;
            ConvertWideCharToMultiByte(p, path);
            std::string a;
            ConvertWideCharToMultiByte(a, args);
            DebugPrint("RunProcess() has output_file specified but no data: \"%s\" \"%s\"", p.c_str(), a.c_str());
        }
        else
        {
            ZoneScopedN("Output File");
            std::fstream file(output_file->data, std::ios_base::out);
            if (!file.good())
            {
                std::string of;
                ConvertWideCharToMultiByte(of, output_file->data);
                DebugPrint("Failed to open file for write: %s", of.c_str());
                FAIL;
                r = ERROR_TOO_MANY_OPEN_FILES;
            }
            else
            {
                TRACY_LOCK(output_file->lock);
                file << output;
            }
        }
    }

    if (!(flags & RunProcess_Async))
        DWORD result = WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(readPipe);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return r;
}

#define TRACY_SET_NAME_FOR_JOB(app, args)\
    ZoneScoped;                                                                         \
    const std::wstring cmdlinew = app.size() ? app + L" " + args : args;                \
    std::string cmdline;                                                                \
    ConvertWideCharToMultiByte(cmdline, cmdlinew);                                      \
    cmdline.find_first_of('')\
    const std::string zone_name = cmdline;                                              \
    ZoneName(zone_name.c_str(), zone_name.size())                                       \
    \
    ZoneText()
    //std::string zone_name = ToString("Process Log To File Job: %s", cmdline.c_str());

void ParsePowershell(PowershellResponse& out, const std::string& in)
{
    const i32 titles_index = 1;
    const i32 lines_index = 2;
    const i32 data_start_index = 3;
    if (data_start_index >= in.size())
        return;
    if (in[0] != '\r' || in[1] != '\n')
        return;
    const std::vector<std::string> strings = TextToStringArray(in.c_str(), "\n");
    if (strings.size() < 3)
        return;
    const std::string& titles = strings[titles_index];
    const std::string& lines = strings[lines_index];
    if (lines[0] != '-')
        return;

    std::vector<i32> column_lengths;
    for (i32 i = 0; i < lines.size(); i++)
    {
        i32 dash_count = i;
        for (; dash_count < lines.size(); dash_count++)
        {
            if (lines[dash_count] != '-')
                break;
        }

        i32 column_width = dash_count;
        for (; column_width < lines.size(); column_width++)
        {
            if (lines[column_width] != ' ')
                break;
        }
        i32 width = column_width - i;
        if (width <= 0)
            break;
        column_lengths.push_back(width);
        i = column_width - 1;
    }
    
    const i32 max_rows = (i32)strings.size() - 2;
    const size_t max_column = Min(std::tuple_size_v<PowershellResponse::value_type>, column_lengths.size());

    i32 out_index = 0;
    for (i32 strings_index = titles_index; strings_index < max_rows; strings_index++)
    {
        if (strings_index == lines_index)
            continue;
        const std::string& s = strings[strings_index];
        i32 previous_len = 0;
        out.push_back({});
        for (i32 j = 0; j < max_column; j++)
        {
            const i32 len = column_lengths[j];
            std::string& s_out = out[out_index][j];
            s_out = s.substr(previous_len, len);
            StringRemoveLeading(s_out, ' ');
            StringRemoveTrailing(s_out, ' ');
            previous_len += len;
        }
        ++out_index;
    }
}

void ParseSysinfo(PowershellResponse& out, const std::string& in)
{
    const i32 data_start_index = 1;
    if (data_start_index >= in.size())
        return;
    if (in[0] != '\r' || in[1] != '\n')
        return;
    const std::vector<std::string> strings = TextToStringArray(in.c_str(), "\n");
    if (strings.size() < 3)
        return;
    const std::string& ds = strings[data_start_index];

    i32 item_len = 0;
    {
        i32 name_count = 0;
        for (; name_count < ds.size(); name_count++)
        {
            if (ds[name_count] == ':')
            {
                name_count++;
                break;
            }
        }

        i32 column_width = name_count;
        for (; column_width < ds.size(); column_width++)
        {
            if (ds[column_width] != ' ')
                break;
        }
        item_len = column_width;
    }
    
    const i32 rows_max = (i32)strings.size() - 1;
    const size_t max_column = 2;

    out.push_back({});
    out[0][0] = "Item";
    out[0][1] = "Value";
    i32 out_index = 1;
    for (i32 rows_i = data_start_index; rows_i < rows_max; rows_i++)
    {
        const std::string& s = strings[rows_i];
        out.push_back({});
        //Add Item
        std::string& item = out[out_index][0];
        item = s.substr(0, item_len);
        StringRemoveLeading(item, ' ');
        StringRemoveTrailing(item, ' ');
        //Add Value
        std::string& value = out[out_index][1];
        value = s.substr(item_len, s.size() - 1);
        StringRemoveTrailing(value, ' ');
        ++out_index;
    }
}

void ParseCSV(PowershellResponse& out, const std::string& in, bool using_quotes)
{
    ZoneScoped;
    if (!in.size())
    {
        FAIL;
        return;
    }
    const std::vector<std::string> rows = TextToStringArray(in.c_str(), "\n");
    if (!rows.size())
    {
        FAIL;
        return;
    }

    for (i32 row_i = 0; row_i < rows.size(); row_i++)
    {
        const std::string& row = rows[row_i];
        std::vector<std::string> strings;
        if (using_quotes)
            strings = TextCsvToStringArray(row.c_str());
        else
            strings = TextToStringArray(row.c_str(), ",");
        if (!strings.size())
            continue;
        out.push_back({});
        if (strings.size() >= PWSH_MAX_COLUMNS)
        {
            FAIL;
            continue;
        }
        for (i32 i = 0; i < strings.size(); i++)
        {
            if (strings[i].size())
            {
                std::string& s = out[out.size() - 1][i];
                s = strings[i];
                if (using_quotes)
                    continue;
#if 0
                //This seems to be marginally faster?
                if (i == strings.size() - 1)
                    s = strings[i].substr(0, strings[i].size() - 2);
                else
                    s = strings[i].substr(0, strings[i].size() - 1);
#else
                s = strings[i];
                //TextRemoval(s, "\"");
                TextRemoval(s, ",");
                TextRemoval(s, "\r");
                TextRemoval(s, "\n");
                StringRemoveTrailing(s, ' ');
                StringRemoveLeading(s, ' ');
#endif
            }
        }
    }
}

void RunProcessJob::RunJob()
{
    std::string zone_text;
    std::string zone_name;
    GetNameAndTextForJob(zone_text, zone_name, path, args);
    ZoneScoped;
    ZoneName(zone_name.c_str(), zone_name.size());
    ZoneText(zone_text.c_str(), zone_text.size());
    const wchar_t* wpath = path.size() ? path.c_str() : nullptr;
    const wchar_t* wargs = args.size() ? args.c_str() : nullptr;
    i32 result = RunProcess(wpath, wargs, output);
    //if (result)
    //{
    //    Threading::GetInstance().RunAndClearJobs();
    //}
}

void RunProcessLogToFileJob::RunJob()
{
    ZoneScopedN("RunProcessLogToFileJob");
    bool r = RunProcess(path.c_str(), args.c_str(), output, &output_file);
    if (run_and_clear && r)
    {
        ZoneScopedN("Run and Clear");
        Threading::GetInstance().RunAndClearJobs();
    }

    if (completed)
    {
        ASSERT(*completed == false);
        (*completed) = true;
    }
}

void* OSGetWindowHandle(SDL_Window* window)
{
    SDL_PropertiesID props = SDL_GetWindowProperties(window);
    HWND hwnd = (HWND)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
    return hwnd;
}

bool OSInit(SDL_Window* window)
{
    //HMODULE modh = GetModuleHandle(NULL);
    //VALIDATE_V(modh != NULL, false);

    HWND hwnd = (HWND)OSGetWindowHandle(window);
    if (!hwnd)
    {
        DebugPrint("Failed to get HWND: %s", SDL_GetError());
        FAIL;
        return false;
    }

    {
        DWORD name_size = MAX_COMPUTERNAME_LENGTH + 1;
        g_sysinfo.name.resize(name_size);
        if (!GetComputerNameW(g_sysinfo.name.data(), &name_size))
        {
            DebugPrint("Failed to get Computer Name error: %i", GetLastError());
            FAIL;
            return false;
        }
        g_sysinfo.name.resize(name_size);
    }

    {
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION info[1024] = {};
        DWORD buffer_size = sizeof(info);
        if (!GetLogicalProcessorInformation(info, &buffer_size))
        {
            DebugPrint("Failed to get processor information error: %i", GetLastError());
            FAIL;
            return false;
        }

        i32 count = buffer_size / sizeof(_SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
        g_sysinfo.cores = 0;
        g_sysinfo.threads = 0;

        for (i32 i = 0; i < count; ++i)
        {
            if (info[i].Relationship == RelationProcessorCore)
            {
                g_sysinfo.cores++;
                ULONG_PTR mask = info[i].ProcessorMask;
                while (mask)
                {
                    g_sysinfo.threads += (mask & 1);
                    mask >>= 1;
                }
            }
        }

        if (g_sysinfo.cores == 0 || g_sysinfo.threads == 0)
        {
            DebugPrint("Error getting cpu and thread counts: %i %i", g_sysinfo.cores, g_sysinfo.threads);
        }
    }
    return true;
}

void OSDestroy(SDL_Window* window)
{
    SDL_DestroyWindow(window);
}

//#pragma comment(lib, "iphlpapi.lib")
//#pragma comment(lib, "ws2_32.lib")

bool OSGetNetworkAdapters(std::vector<OSNetworkAdapterInfo>& adapters)
{
    ZoneScoped;
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data))
    {
        DebugPrint("Error: WSAStartup failed");
        return false;
    }

    ULONG buf_len = Kilobytes(15);
    std::vector<u8> buffer(buf_len);
    PIP_ADAPTER_ADDRESSES adapter_addresses = (PIP_ADAPTER_ADDRESSES)buffer.data();

    ULONG flags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_INCLUDE_WINS_INFO | GAA_FLAG_INCLUDE_GATEWAYS;
    DWORD r = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, adapter_addresses, &buf_len);
    if (r == ERROR_BUFFER_OVERFLOW)
    {
        buffer.resize(buf_len);
        adapter_addresses = (PIP_ADAPTER_ADDRESSES)buffer.data();
        r = GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, adapter_addresses, &buf_len);
    }
    if (r != ERROR_SUCCESS)
    {
        DebugPrint("Error Failed to GetAdaptersAddresses()");
        return false;
    }

    PIP_ADAPTER_ADDRESSES adapter = adapter_addresses;
    while (adapter)
    {
        OSNetworkAdapterInfo ad = {};
        ad.name = adapter->AdapterName;
        ad.friendly_name = adapter->FriendlyName;
        ad.description = adapter->Description;
        ad.status = adapter->OperStatus == IfOperStatusUp ? "Up" : "Down";
        ad.ipv4_enabled = adapter->Ipv4Enabled;
        ad.ipv6_enabled = adapter->Ipv6Enabled;
        ad.dhcpv4_enabled = adapter->Dhcpv4Enabled;
        ad.ipv4_metric = adapter->Ipv4Metric;
        ad.ipv6_metric = adapter->Ipv6Metric;
        ad.ddns_enabled = adapter->DdnsEnabled;
        ad.domain_dns_register_enabled = adapter->RegisterAdapterSuffix;
        ad.receive_only = adapter->ReceiveOnly;
        ad.multicast_enabled = !adapter->NoMulticast;

        if (adapter->PhysicalAddressLength != 0)
        {
            for (ULONG i = 0; i < adapter->PhysicalAddressLength; i++)
            {
                char s[4] = {};
                sprintf(s, "%.2X%s", adapter->PhysicalAddress[i], (i == (adapter->PhysicalAddressLength - 1)) ? "" : "-");
                ad.mac_address = ad.mac_address + s;
            }
        }

        // Get IP Addresses (Unicast)
        PIP_ADAPTER_UNICAST_ADDRESS unicast = adapter->FirstUnicastAddress;
        while (unicast)
        {
            OSIPAndSubnet ips = {};
            char ip_str[INET6_ADDRSTRLEN] = {};
            sockaddr* sa = unicast->Address.lpSockaddr;
            const u8 prefix_len = unicast->OnLinkPrefixLength;
            if (sa)
            {
                if (sa->sa_family == AF_INET) //IPv4
                {
                    sockaddr_in* sa_in = (sockaddr_in*)sa;
                    inet_ntop(AF_INET, &(sa_in->sin_addr), ip_str, sizeof(ip_str));
                    ips.ip = ip_str;

                    //Bitwise math to create the mask like 255.255.255.0
                    const u32 mask = (prefix_len == 0) ? 0 : (~0UL << (32 - prefix_len));

                    in_addr mask_addr;
                    mask_addr.s_addr = htonl(mask);
                    char mask_s[INET_ADDRSTRLEN] = {};
                    inet_ntop(AF_INET, &mask_addr, mask_s, sizeof(mask_s));
                    ips.subnet = ToString("%s(/%i)", mask_s, prefix_len);

                    ad.ipv4_ips.push_back(ips);
                }
                else if (sa->sa_family == AF_INET6) //IPv6
                {
                    sockaddr_in6* sa_in = (sockaddr_in6*)sa;
                    inet_ntop(AF_INET6, &(sa_in->sin6_addr), ip_str, sizeof(ip_str));
                    ips.ip = ip_str;
                    ips.subnet = ToString("/%i", prefix_len);
                    ad.ipv6_ips.push_back(ips);
                }
            }

            unicast = unicast->Next;
        }

        // Get DNS Servers
        PIP_ADAPTER_DNS_SERVER_ADDRESS dns = adapter->FirstDnsServerAddress;
        while (dns)
        {
            char dns_str[INET6_ADDRSTRLEN] = {};
            sockaddr* sa = dns->Address.lpSockaddr;

            if (sa)
            {
                if (sa->sa_family == AF_INET) //IPv4
                {
                    sockaddr_in* sa_in = (sockaddr_in*)sa;
                    inet_ntop(AF_INET, &(sa_in->sin_addr), dns_str, sizeof(dns_str));
                    ad.ipv4_dns.push_back(dns_str);
                }
                else if (sa->sa_family == AF_INET6) //IPv6
                {
                    sockaddr_in6* sa_in = (sockaddr_in6*)sa;
                    inet_ntop(AF_INET6, &(sa_in->sin6_addr), dns_str, sizeof(dns_str));
                    ad.ipv6_dns.push_back(dns_str);
                }
            }
            dns = dns->Next;
        }

        // Get DNS Domain/Suffix
        if (wcslen(adapter->DnsSuffix) > 0)
        {
            ad.dns_domain = adapter->DnsSuffix;
        }

        // Get Default Gateways
        PIP_ADAPTER_GATEWAY_ADDRESS gateway = adapter->FirstGatewayAddress;
        while (gateway)
        {
            char g_s[INET6_ADDRSTRLEN] = { };
            sockaddr* sa = gateway->Address.lpSockaddr;
            if (sa)
            {
                if (sa->sa_family == AF_INET)
                {
                    sockaddr_in* sa_in = (sockaddr_in*)sa;
                    inet_ntop(AF_INET, &(sa_in->sin_addr), g_s, sizeof(g_s));
                    ad.ipv4_gateways.push_back(g_s);
                }
                else if (sa->sa_family == AF_INET6)
                {
                    sockaddr_in6* sa_in = (sockaddr_in6*)sa;
                    inet_ntop(AF_INET6, &(sa_in->sin6_addr), g_s, sizeof(g_s));
                    ad.ipv6_gateways.push_back(g_s);
                }
            }
            gateway = gateway->Next;
        }

        // Get DHCP
        ASSERT(ad.dhcpv4_enabled == !!(adapter->Flags & IP_ADAPTER_DHCP_ENABLED));
        if (adapter->Flags & IP_ADAPTER_DHCP_ENABLED)
        {
            char dhcp_s[INET6_ADDRSTRLEN] = {};
            sockaddr* sa = adapter->Dhcpv4Server.lpSockaddr;
            if (sa)
            {
                if (sa->sa_family == AF_INET)
                {
                    sockaddr_in* sa_in = (sockaddr_in*)sa;
                    inet_ntop(AF_INET, &(sa_in->sin_addr), dhcp_s, sizeof(dhcp_s));
                    ad.ipv4_dhcp = dhcp_s;
                }
                else if (sa->sa_family == AF_INET6)
                {
                    sockaddr_in6* sa_in = (sockaddr_in6*)sa;
                    inet_ntop(AF_INET6, &(sa_in->sin6_addr), dhcp_s, sizeof(dhcp_s));
                    ad.ipv6_dhcp = dhcp_s;
                }
            }
        }

        adapters.push_back(ad);
        adapter = adapter->Next;
    }

    WSACleanup();
    return true;
}

void SysProcessEvents()
{
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    ZoneScopedN("Poll Events");
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        ImguiProcessEvent(&event);
        //DebugPrint("Event: %i", event.type);
        if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(gfx.window))
            g_running = true;

        switch (event.type)
        {
        case SDL_EVENT_QUIT:
            g_running = false;
            break;
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            g_running = !(event.window.windowID == SDL_GetWindowID(gfx.window));
            break;
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
            g_sysinfo.keys[event.key.key].down = (event.type == SDL_EVENT_KEY_DOWN);
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
            g_sysinfo.keys[event.button.button].down = event.button.down;
            break;
        case SDL_EVENT_MOUSE_MOTION:
        {
            ZoneScopedN("SDL_MOUSEMOTION");
            Vec2 delta;
            delta.x = ((event.motion.x) - g_sysinfo.mouse.p.x);
            delta.y = ((event.motion.y) - g_sysinfo.mouse.p.y);

            g_sysinfo.mouse.delta_p += delta;
            g_sysinfo.mouse.p.x = event.motion.x;
            g_sysinfo.mouse.p.y = event.motion.y;
            break;
        }
        case SDL_EVENT_MOUSE_WHEEL:
        {
            g_sysinfo.mouse.wheel_instant.x = g_sysinfo.mouse.wheel.x = event.wheel.x;
            g_sysinfo.mouse.wheel_instant.y = g_sysinfo.mouse.wheel.y = event.wheel.y;
            break;
        }
        case SDL_EVENT_WINDOW_RESIZED:
        {
            gfx.window_size.x = event.window.data1;
            gfx.window_size.y = event.window.data2;
            break;
        }
        case SDL_EVENT_WINDOW_FOCUS_GAINED:
        {
            g_sysinfo.has_attention = true;
            //g_sysinfo.mouse.delta_p = {};
            //SDL_GetMouseState(&g_sysinfo.mouse.p.x, &g_sysinfo.mouse.p.y);
            //g_sysinfo.mouse.p.y = g_settings.graphics.resolution.y - g_sysinfo.mouse.p.y;
            //SetFocus(g_renderer.SDL_Context);
            break;
        }
        case SDL_EVENT_WINDOW_FOCUS_LOST:
        {
            g_sysinfo.has_attention = false;
            break;
        }
        case SDL_EVENT_DROP_BEGIN:
            g_sysinfo.drop_active = true;
            break;
        case SDL_EVENT_DROP_COMPLETE:
            g_sysinfo.drop_active = false;
            break;
        case SDL_EVENT_DROP_FILE:
            if (event.drop.data)
            {
                g_sysinfo.drop_file = event.drop.data;
            }
            break;
        //case SDL_EVENT_DROP_TEXT:
        //case SDL_EVENT_DROP_BEGIN:
        //case SDL_EVENT_DROP_COMPLETE:
        //case SDL_EVENT_DROP_POSITION:
        //{
        //    if (event.drop.file)
        //    {
        //    }
        //    break;
        //}
        }
    }

    for (auto& key : g_sysinfo.keys)
    {
        if (key.second.down)
        {
            key.second.upThisFrame = false;
            if (key.second.downPrevFrame)
            {
                key.second.downThisFrame = false;
            }
            else
            {
                key.second.downThisFrame = true;
            }
        }
        else
        {
            key.second.downThisFrame = false;
            if (key.second.downPrevFrame)
            {
                key.second.upThisFrame = true;
            }
            else
            {
                key.second.upThisFrame = false;
            }
        }
        key.second.downPrevFrame = key.second.down;
    }

    if (g_sysinfo.mouse.wheel_modified_last_frame)
    {
        g_sysinfo.mouse.wheel_instant.y = 0;
        g_sysinfo.mouse.wheel_modified_last_frame = false;
    }
    else if (g_sysinfo.mouse.wheel_instant.y)
    {
        g_sysinfo.mouse.wheel_modified_last_frame = true;
    }
}

bool ConsoleAttached()
{
    return AttachConsole(ATTACH_PARENT_PROCESS);
}
bool DebuggerAttached()
{
    return IsDebuggerPresent();
}

//class DebugStreamBuffer final : public std::streambuf
//{
//protected:
//    int overflow(int c) override
//    {
//        if (c != EOF)
//        {
//            OutputDebugStringA((char*)&c);
//        }
//        return c;
//    }
//};
//static DebugStreamBuffer g_debug_stream_buffer;
//void EnableOutputToDebugger()
//{
//    std::cerr.rdbuf(&g_debug_stream_buffer);
//    std::cout.rdbuf(&g_debug_stream_buffer);
//}

void HideConsole()
{
    ::ShowWindow(::GetConsoleWindow(), SW_HIDE);
}

void ShowConsole()
{
    ::ShowWindow(::GetConsoleWindow(), SW_SHOW);
}

bool IsConsoleVisible()
{
    return ::IsWindowVisible(::GetConsoleWindow()) != FALSE;
}

void SysSleep(u64 _ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(_ms));
}

double SysGetTime()
{
    const static double freq = double(SDL_GetPerformanceFrequency()); //HZ
    double time = SDL_GetPerformanceCounter() / freq;
    return time;
}
float SysMonitorScale()
{
    const static float scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    return scale;
}

i32 ShowCustomErrorWindow(const std::string& title, const std::string& text)
{
    FAIL;
    //const SDL_MessageBoxButtonData buttons[] = {
    //    { 0,                                        MessageBoxResponse_Quit, "Quit Program" },
    //    { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,  MessageBoxResponse_Continue, "Continue" },
    //    { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,  MessageBoxResponse_OpenLog, "Open Log" },
    //};
    //const SDL_MessageBoxColorScheme colorScheme = {
    //    { /* .colors (.r, .g, .b) */
    //        /* [SDL_MESSAGEBOX_COLOR_BACKGROUND] */
    //        { 255,   0,   0 },
    //        /* [SDL_MESSAGEBOX_COLOR_TEXT] */
    //        {   0, 255,   0 },
    //        /* [SDL_MESSAGEBOX_COLOR_BUTTON_BORDER] */
    //        { 255, 255,   0 },
    //        /* [SDL_MESSAGEBOX_COLOR_BUTTON_BACKGROUND] */
    //        {   0,   0, 255 },
    //        /* [SDL_MESSAGEBOX_COLOR_BUTTON_SELECTED] */
    //        { 255,   0, 255 }
    //    }
    //};
    //const SDL_MessageBoxData messageboxdata = {
    //    //SDL_MESSAGEBOX_INFORMATION, /* .flags */
    //    //SDL_MESSAGEBOX_ERROR,
    //    SDL_MESSAGEBOX_WARNING,
    //    NULL, /* .window */
    //    title.c_str(), /* .title */
    //    text.c_str(), /* .message */
    //    SDL_arraysize(buttons), /* .numbuttons */
    //    buttons, /* .buttons */
    //    &colorScheme /* .colorScheme */
    //};
    //i32 buttonID = -1;
    //if (SDL_ShowMessageBox(&messageboxdata, &buttonID) < 0) {
    //    SDL_Log("error displaying message box");
    //    //Quit Program
    //    SDL_Event e;
    //    e.type = SDL_QUIT;
    //    e.quit.timestamp = 0;
    //    SDL_PushEvent(&e);
    //    return 0;
    //}
    ////TODO: Add better error handling for this?
    //ASSERT(buttonID >= 0);

    //if (buttonID == MessageBoxResponse_Quit)
    //{
    //    SDL_Event e;
    //    e.type = SDL_QUIT;
    //    e.quit.timestamp = 0;
    //    SDL_PushEvent(&e);
    //}
    //return buttonID;
    return 1;
}

void ShowErrorWindow(const std::wstring& title, const std::wstring& text)
{
#if 1
    int msgboxID = MessageBox(
        NULL,
        text.c_str(),
        title.c_str(),
        MB_ABORTRETRYIGNORE | MB_ICONSTOP | MB_DEFBUTTON1 | MB_APPLMODAL
    );

    switch (msgboxID)
    {
    case IDABORT:
        g_running = false;
        break;
    case IDRETRY:
        break;
    case IDIGNORE:
        break;
    }
#else
    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings;
    const ImVec2 min = { 260, 100 };
    const ImVec2 windowSize = ImGui::GetMainViewport()->WorkSize;
    const ImVec2 max = {windowSize.x - 200, windowSize.y - 200};
    ImGui::SetNextWindowSizeConstraints(min, max);
    ImGui::SetNextWindowPos(ImVec2(windowSize.x / 2, windowSize.y / 2), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::OpenPopup(title.c_str());
    if (ImGui::BeginPopupModal(title.c_str(), NULL, flags))
    {
        ImGui::TextWrapped(text.c_str());
        if (ImGui::Button("Continue"))
            ImGui::CloseCurrentPopup();
        ImGui::SameLine();
        if (ImGui::Button("Copy to Clipboard"))
            SDL_SetClipboardText(text.c_str());
        ImGui::SameLine();
        if (ImGui::Button("Exit"))
        {
            SDL_Event e;
            e.type = SDL_QUIT;
            e.quit.timestamp = 0;
            SDL_PushEvent(&e);
        }
        ImGui::EndPopup();
    }
#endif
}
void SysFlashWindow(SDL_Window* window)
{
    FLASHWINFO info = {};
    info.hwnd = (HWND)OSGetWindowHandle(window);
    info.dwFlags = FLASHW_TRAY | FLASHW_TIMERNOFG;
    info.uCount;
    info.dwTimeout;
    info.cbSize = sizeof(info);

    FlashWindowEx(&info);
}

//TODO(CSH): Create filepath helper functions
void _ScanDirectoryForFileNames(const std::wstring& root, const std::wstring& dir, ScannedFiles& out, ScanDirectoryFlags flags)
{
    std::wstring d = root;
    if (d.size() < 2)
    {
        d = L"*";
    }
    else
    {

        size_t end_index = d.find_first_of(L'\0');
        if (end_index == std::wstring::npos)
            end_index = d.size();
        if (d[end_index - 1] != L'*')
        {
            if (d[end_index - 1] != L'/')
            {
                d.insert(end_index, L"/*");
            }
            else
            {
                d.insert(end_index, L"*");
            }
        }
    }

    WIN32_FIND_DATAW find_data;
    HANDLE handle = FindFirstFileW(d.c_str(), &find_data);
    if (handle == INVALID_HANDLE_VALUE)
    {
        DWORD error = GetLastError();
        std::string mb;
        ConvertWideCharToMultiByte(mb, d);
        DebugPrint(ToString("Error finding files: %s", mb.c_str()).c_str());
        return;
    }
    while (handle != INVALID_HANDLE_VALUE)
    {
        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && find_data.cFileName[0] != '.')
        {
            if (flags & ScanDirectoryFlags_IncludeDirs)
            {
                if (dir.size())
                    out.push_back({ dir + L"/" + find_data.cFileName, true });
                else
                    out.push_back({ find_data.cFileName, true });
            }

            if (flags & ScanDirectoryFlags_Recursive)
            {
                std::wstring new_root = d;
                new_root.pop_back();
                new_root += find_data.cFileName;
                std::wstring new_dir = dir;
                if (dir.size())
                    new_dir = new_dir + L"/" + find_data.cFileName;
                else
                    new_dir += find_data.cFileName;
                _ScanDirectoryForFileNames(new_root, new_dir, out, flags);
            }
        }
        else if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
                if (dir.size())
                    out.push_back({ dir + L"/" + find_data.cFileName, false });
                else
                    out.push_back({ find_data.cFileName, false });
        }
        if (FindNextFileW(handle, &find_data) == 0)
        {
            //if (GetLastError() == ERROR_NO_MORE_FILES)
            break;
        }
    }
}

void ScanDirectoryForFileNames(const std::wstring& dir, ScannedFiles& out, ScanDirectoryFlags flags)
{
    out.clear();
    _ScanDirectoryForFileNames(dir, L"", out, flags);
}

#include "shlobj_core.h"

static int CALLBACK BrowseFolderCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    if (uMsg == BFFM_INITIALIZED) {
        LPCTSTR path = LPCTSTR(lpData);
        ::SendMessage(hwnd, BFFM_SETSELECTION, true, (LPARAM)path);
    }
    return 0;
}

bool GetDirectoryFromUser(const std::wstring& currentDir, std::wstring& dir)
{
    std::wstring baseDir = currentDir;
    if (currentDir.size() == 0)
    {
        TCHAR buf[MAX_PATH] = { 0 };
        GetModuleFileName(NULL, buf, MAX_PATH);
        std::wstring::size_type pos = std::wstring(buf).find_last_of(L"\\/");
        baseDir = std::wstring(buf).substr(0, pos);
    }
    dir.clear();
    dir.resize(MAX_PATH);
    int imageIndex = 0;
    BROWSEINFO info = {
        .hwndOwner = (HWND)OSGetWindowHandle(gfx.window),
        .pidlRoot = NULL,
        .pszDisplayName = NULL,//dir.data(),
        .lpszTitle = L"Select Config Directory",
        .ulFlags =  BIF_USENEWUI, //BIF_EDITBOX | BIF_NEWDIALOGSTYLE,
        .lpfn = BrowseFolderCallback,//NULL,
        .lParam = (LPARAM)baseDir.c_str(), //NULL,
        .iImage = imageIndex,
    };
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    PIDLIST_ABSOLUTE pidl = SHBrowseForFolder(&info);
    if (pidl == NULL)
        return false;
    BOOL result = SHGetPathFromIDList(pidl, dir.data());

    auto pos = dir.find_first_of(L'\0');
    if (pos != std::wstring::npos)
        dir.resize(pos);

    if (result)
    {
        std::wstring_view dir1 = dir;
        std::wstring_view dir2 = baseDir;
        if (dir1.find(dir2) != std::wstring::npos && dir2.find(dir1) != std::wstring::npos)
        {
            dir.clear();
        }
        return true;
    }
    return false;
}

void ConvertMultibyteToWideChar(std::wstring& out, const std::string& in)
{
    //WideCharToMultiByte
    i32 wide_char_count = MultiByteToWideChar(
        CP_UTF8,                //[in]            UINT                              CodePage,
        MB_ERR_INVALID_CHARS,   //[in]            DWORD                             dwFlags,
        in.c_str(),             //[in]            _In_NLS_string_(cbMultiByte)LPCCH lpMultiByteStr,
        -1,                     //[in]            int                               cbMultiByte,
        nullptr,                //[out, optional] LPWSTR                            lpWideCharStr,
        0                       //[in]            int                               cchWideChar
    );
    ASSERT(wide_char_count > 0);
    out.clear();
    out.resize(wide_char_count);
    i32 wide_char_actual = MultiByteToWideChar(
        CP_UTF8,                //[in]            UINT                              CodePage,
        MB_ERR_INVALID_CHARS,   //[in]            DWORD                             dwFlags,
        in.c_str(),             //[in]            _In_NLS_string_(cbMultiByte)LPCCH lpMultiByteStr,
        -1,                     //[in]            int                               cbMultiByte,
        out.data(),             //[out, optional] LPWSTR                            lpWideCharStr,
        wide_char_count         //[in]            int                               cchWideChar
    );
    ASSERT(wide_char_actual > 0);
    ASSERT(wide_char_actual == wide_char_count);
}

void ConvertWideCharToMultiByte(std::string& out, const std::wstring& in)
{
    //WideCharToMultiByte
    BOOL invalid_string;

    i32 multibyte_char_count = WideCharToMultiByte(
        CP_UTF8,                //[in]            UINT                               CodePage,
        0,//MB_ERR_INVALID_CHARS,   //[in]            DWORD                              dwFlags,
        in.c_str(),             //[in]            _In_NLS_string_(cchWideChar)LPCWCH lpWideCharStr,
        -1,                     //[in]            int                                cchWideChar,
        nullptr,                //[out, optional] LPSTR                              lpMultiByteStr,
        0,                      //[in]            int                                cbMultiByte,
        "#",                    //[in, optional]  LPCCH                              lpDefaultChar,
        &invalid_string         //[out, optional] LPBOOL                             lpUsedDefaultChar
    );
    ASSERT(multibyte_char_count > 0);
    out.clear();
    out.resize(multibyte_char_count);
    i32 multibyte_char_actual = WideCharToMultiByte(
        CP_UTF8,                //[in]            UINT                               CodePage,
        0,//MB_ERR_INVALID_CHARS,   //[in]            DWORD                              dwFlags,
        in.c_str(),             //[in]            _In_NLS_string_(cchWideChar)LPCWCH lpWideCharStr,
        -1,                     //[in]            int                                cchWideChar,
        out.data(),             //[out, optional] LPSTR                              lpMultiByteStr,
        (i32)out.size(),        //[in]            int                                cbMultiByte,
        "#",                    //[in, optional]  LPCCH                              lpDefaultChar,
        &invalid_string         //[out, optional] LPBOOL                             lpUsedDefaultChar
    );
    ASSERT(multibyte_char_actual > 0);
    ASSERT(multibyte_char_actual == multibyte_char_count);
}

void ExpandEnvironemntVariable(std::wstring& out, const std::wstring& in)
{
    DWORD size = ExpandEnvironmentStringsW(in.c_str(), nullptr, 0);
    if (size == 0)
    {
        std::string var;
        ConvertWideCharToMultiByte(var, in);
        DebugPrint("Failed to expand string: \"%s\" error: %i", var.c_str(), GetLastError());
        return;
    }

    out.resize(size);
    ExpandEnvironmentStringsW(in.c_str(), out.data(), size);
    out.resize(size - 1); //Remove trailing null inserted by API
}

void ToLower(std::wstring& s)
{
    std::transform(s.begin(), s.end(), s.begin(),
        [](std::wint_t c) { return std::towlower(c); }
    );
    FAIL; //untested
}

void ToLower(std::string& s)
{
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return std::tolower(c); }
    );
}

#if FEATURE_CUSTOM_ASSERT
#pragma comment(lib, "Comctl32.lib")
#include <commctrl.h>
#include <signal.h> // raise

#if WINVER <= _WIN32_WINNT_WINXP
#define SRW_LOCK_INIT(_lock) 
#define SRW_LOCK_ACQUIRE(_lock) _lock.lock()
#define SRW_LOCK_RELASE(_lock) _lock.unlock()
#define SRW_LOCK std::mutex
#define ASSERT_DIALOG(_message, _info, _button) _button = MessageBoxW(NULL,\
                                                                   _info,\
                                                                   _message,\
                                                                   MB_CANCELTRYCONTINUE | MB_ICONWARNING | MB_DEFBUTTON2)
                                                           
#else
#define SRW_LOCK_INIT(_lock) InitializeSRWLock(_lock)
#define SRW_LOCK_ACQUIRE(_lock) AcquireSRWLockExclusive(&_lock)
#define SRW_LOCK_RELASE(_lock) ReleaseSRWLockExclusive(&_lock)
#define SRW_LOCK SRWLOCK
#define ASSERT_DIALOG(_message, _info, _button) TaskDialog(NULL, NULL, \
                                                           L"Assertion Failed",\
                                                           _message,\
                                                           _info,\
                                                           TDCBF_YES_BUTTON | TDCBF_NO_BUTTON | TDCBF_RETRY_BUTTON | TDCBF_CLOSE_BUTTON, TD_WARNING_ICON,\
                                                           &_button)
#endif

struct AssertRecord
{
    // Key
    const char* file; // Points to what is retrieved from the __FILE__ macro, so it should be stable.
    int         line;

    int         hit_counter;
    bool        ignored;
};


struct SRWLock
{
    SRWLock() { SRW_LOCK_INIT(&lock); }
    SRW_LOCK lock;
};

static SRWLock s_assert_mutex;
static std::vector<AssertRecord> s_assert_records;
void OsAssert(bool expr, const char* message, const char* file, int line)
{
    if (!expr)
    {
        SRW_LOCK_ACQUIRE(s_assert_mutex.lock);
        Defer { SRW_LOCK_RELASE(s_assert_mutex.lock); };

        AssertRecord* record = nullptr;
        for (AssertRecord& it : s_assert_records)
        {
            if (it.file == file && it.line == line)
            {
                record = &it;
                break;
            }
        }

        if (!record)
        {
            AssertRecord new_record = {
                .file = file,
                .line = line,
            };
            s_assert_records.push_back(new_record);
            record = &s_assert_records.back();
            record->file = file;
            record->line = line;
        }

        record->hit_counter++;
        if (record->ignored)
        {
            return;
        }


        WCHAR wmessage[1024];
        ArrayView<WCHAR> wmessage_view = CreateArrayView(wmessage);
        wmessage_view.Last() = 0;
        if (MultiByteToWideChar(CP_UTF8, 0, message, -1, wmessage_view.data, (int)wmessage_view.Bytes()) == 0)
        {
            wcscpy_s(wmessage, (size_t)wmessage_view.Bytes(), L"Error");
        }


        const char* s = record->hit_counter == 1 ? "" : "s";
        char info_buffer[1024];
        ArrayView<char> info_buffer_view = CreateArrayView(info_buffer);
#if WINVER == 0x0501
        sprintf_s(info_buffer_view.data, (size_t)info_buffer_view.Bytes(), "%s(%d)\n\n"
                                                     "This has been hit %d time%s.\n\n"
                                                     "Cancel    : Break into debugger\n"
                                                     "Try Again : Continue execution\n"
                                                     "Continue  : Ignore this assert in the future",
                                                     file, line, record->hit_counter, s);
#else
        sprintf_s(info_buffer_view.data, (size_t)info_buffer_view.Bytes(), "%s(%d)\n\n"
                                                     "This has been hit %d time%s.\n\n"
                                                     "Yes   : Break into debugger\n"
                                                     "No    : Continue execution\n"
                                                     "Retry : Ignore this assert in the future\n"
                                                     "Close : Abort the program",
                                                     file, line, record->hit_counter, s);
#endif
        WCHAR winfo[1024];
        auto winfo_view = CreateArrayView(winfo);
        winfo_view.Last() = 0;
        if (MultiByteToWideChar(CP_UTF8, 0, info_buffer, -1, winfo_view.data, (int)winfo_view.Bytes()) == 0)
        {
            wcscpy_s(winfo, (size_t)winfo_view.Bytes(), L"Error");
        }


        int button = 0;
        ASSERT_DIALOG(wmessage, winfo, button);


        switch(button)
        {
        default:
        case IDTRYAGAIN: [[fallthrough]];
        case IDNO: break;
#if WINVER == 0x0501
        case IDCANCEL: [[fallthrough]];
#else
        case IDCANCEL: {
        } break;
#endif
        case IDYES: {
            __debugbreak();
        } break;

        case IDCONTINUE: [[fallthrough]];
        case IDRETRY: {
            if (record)
            {
                record->ignored = true;
            }
        } break;

        case IDCLOSE: {
            // NOTE: This is how the CRT assert works when the abort button is pressed:
            raise(SIGABRT);
            _exit(3);
        } break;
        }
    }
}
#else
#include <assert.h>
void os_assert(bool expr, const char*, const char*, int)
{
    ASSERT(expr);
}
#endif

struct OS {
    HMODULE hmod;
    HWND hwnd;
    Vec2I screen_size;
};
static OS s_os;


int main(int argc, char** argv)
{
    return Main(argc, argv);
}
int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR str, int val)
{
    return Main(-1, &str);
}

void ArchiveErrorCheck(archive* a, int e)
{
    if (e != ARCHIVE_OK)
    {
        const char* error_rr_string = archive_error_string(a);
        DebugPrint("Archive Failure: %s", error_rr_string);
        FAIL;
    }
}

void AddEntryToZip(archive* a, const std::filesystem::path& full_path, const std::filesystem::path& relative_path, bool is_dir, std::vector<u8>& file_buffer, std::atomic<u64>& progress)
{
    struct stat st;
    if (stat(full_path.string().c_str(), &st) != 0)
    {
        perror("Problem getting information");
        int r = errno;
        switch (r)
        {
        case ENOENT:
            DebugPrint("File %s not found.\n", full_path.string().c_str());
            break;
        case EINVAL:
            DebugPrint("Invalid parameter to _stat.\n");
            break;
        default:
            //Should never be reached.
            DebugPrint("Unexpected error in _stat.\n");
        }
        FAIL;
        return;
    }
    if (is_dir)
    {
        std::vector<ScannedFile> out;
        ScanDirectoryForFileNames(full_path, out, ScanDirectoryFlags_IncludeDirs);
        for (i32 i = 0; i < (i32)out.size(); i++)
        {
            AddEntryToZip(a, full_path / out[i].name, relative_path / out[i].name, out[i].dir, file_buffer, progress);
        }
    }
    else
    {
        archive_entry* entry = archive_entry_new();
        archive_entry_set_pathname(entry, relative_path.string().c_str());
        archive_entry_set_filetype(entry, AE_IFREG);
        archive_entry_copy_stat(entry, &st);
        int error = archive_write_header(a, entry);
        ArchiveErrorCheck(a, error);

        {
            std::ifstream file(full_path, std::ios::binary | std::ios::ate);
            if (!file)
            {
                DebugPrint("Error opening file: %s", full_path.string().c_str());
                FAIL;
                return;
            }
            const size_t file_size = (size_t)file.tellg();
            if (file_size > file_buffer.size())
                file_buffer.resize(file_size * 2);
            file.seekg(0, std::ios::beg);
            file.read((char*)file_buffer.data(), file_size);

            error = (int)archive_write_data(a, file_buffer.data(), file_size);
            if (error < 0)
                ArchiveErrorCheck(a, error);
            ++progress;
        }
        archive_entry_free(entry);
    }
}

void CreateZip(const Path& zip_path, const Path& source_folder, ArrayView<ScannedFile> files_to_backup, ArrayView<Path> files_to_add_to_root, Atomic<u64>& progress/*, ArrayView<std::wstring> ext_to_exclude*/)
{
    archive* a = archive_write_new();
    archive_write_set_format_zip(a);
    int error = archive_write_zip_set_compression_deflate(a);
    ArchiveErrorCheck(a, error);
    error = archive_write_set_options(a, "compression-level=9");
    ArchiveErrorCheck(a, error);
    CreateParentDirectories(zip_path);
    error = archive_write_open_filename(a, zip_path.string().c_str());
    ArchiveErrorCheck(a, error);

    std::vector<u8> file_buffer;
    //file_buffer.reserve(64*1000*1000);
    for (i32 i = 0; i < files_to_backup.size(); i++)
    {
        Path full;
        if (!source_folder.empty())
            full = source_folder / files_to_backup[i].name;
        else
            full = files_to_backup[i].name;
        AddEntryToZip(a, full, files_to_backup[i].name, files_to_backup[i].dir, file_buffer, progress);
    }
    for (i32 i = 0; i < files_to_add_to_root.size(); i++)
    {
        AddEntryToZip(a, files_to_add_to_root[i], files_to_add_to_root[i].filename(), false, file_buffer, progress);
    }

    error = archive_write_close(a);
    ArchiveErrorCheck(a, error);
    error = archive_write_free(a);
    ArchiveErrorCheck(a, error);
}

bool UnzipArchive(const std::string& zip_path, const std::string& output_dir, std::vector<std::string>& filenames)
{
    struct archive* a = archive_read_new();
    archive_read_support_format_zip(a);
    archive_read_support_filter_all(a);

    int error = ARCHIVE_OK;
    archive_read_open_filename(a, zip_path.c_str(), 10240);
    ArchiveErrorCheck(a, error);

    struct archive* ext = archive_write_disk_new();
    archive_write_disk_set_options(ext,
        ARCHIVE_EXTRACT_TIME |
        ARCHIVE_EXTRACT_PERM |
        ARCHIVE_EXTRACT_ACL |
        ARCHIVE_EXTRACT_FFLAGS);

    struct archive_entry* entry;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK)
    {
        const char* relative_path = archive_entry_pathname(entry);
        std::string full_path;
        if (output_dir.size())
            full_path = output_dir + "/" + relative_path;
        else
            full_path = relative_path;
        archive_entry_set_pathname(entry, full_path.c_str());
        filenames.push_back(full_path);

        archive_write_header(ext, entry);

        const void* buff;
        size_t size;
        la_int64_t offset;

        while (archive_read_data_block(a, &buff, &size, &offset) == ARCHIVE_OK)
        {
            archive_write_data_block(ext, buff, size, offset);
        }

        archive_write_finish_entry(ext);
    }

    archive_write_close(ext);
    archive_write_free(ext);

    archive_read_close(a);
    archive_read_free(a);

    return true;
}

void* OsGetDataFromResource(i32* out_size, const i32 resource_id)
{
    HRSRC handle = FindResource(nullptr, MAKEINTRESOURCE(resource_id), RT_RCDATA);
    DWORD error = GetLastError();
    VALIDATE_V(handle, nullptr);
    HGLOBAL res = LoadResource(nullptr, handle);
    VALIDATE_V(res, nullptr);
    DWORD size = SizeofResource(nullptr, handle);
    if (out_size)
        *out_size = (i32)size;
    void* data = LockResource(res);
    return data;
}

ImFont* LoadFontForImgui(int resource_id, float fontSize)
{
    i32 size;
    void* data = OsGetDataFromResource(&size, resource_id);
    if (!data || size == 0)
        return nullptr;

    ImFontConfig cfg;
    cfg.FontDataOwnedByAtlas = false;
    ImFont* font = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(
        data,
        size,
        fontSize,
        &cfg
    );
    if (!font)
        return nullptr;
    return font;
}

static_assert(sizeof(GUID) == sizeof(Guid));
Guid NewGuid()
{
    Guid id;
    if (FAILED(CoCreateGuid((GUID*)&id)))
    {
        const DWORD error = GetLastError();
        FAIL;
        return {};
    }
    return id;
}

std::string Guid::ToString() const
{
    return ::ToString("%08X-%04X-%04X-%04X-%04X%08X", a, b >> 16, b & 0XFFFF, c >> 16, c & 0XFFFF, d);
}

Guid GuidFromString(const char* s)
{
    Guid r = {};
    const size_t char_len = strnlen_s(s, 38);
    if (char_len != 36)
    {
        FAIL;
        return r;
    }

    char b[8] = { s[ 9], s[10], s[11], s[12], s[14], s[15], s[16], s[17] };
    char c[8] = { s[19], s[20], s[21], s[22], s[24], s[25], s[26], s[27] };
    r.a = (u32)strtoll(s,      nullptr, 16);
    r.b = (u32)strtoll(b,      nullptr, 16);
    r.c = (u32)strtoll(c,      nullptr, 16);
    r.d = (u32)strtoll(&s[28], nullptr, 16);

    return r;
}
