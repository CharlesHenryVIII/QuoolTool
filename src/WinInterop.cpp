#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>
#include <combaseapi.h>

#include "WinInterop.h"
#include "WinInterop_File.h"
#include "Math.h"
#include "String.h"
#include "resource.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
//#include "Windows/resource.h"
#include "Json.hpp"
#define GLFW_EXPOSE_NATIVE_WIN32
#include "glfw/glfw3native.h"

#include <fstream>
#include <filesystem>
#include <cwctype>
#include <format>
#include <fstream>
#include <iostream>
#include <chrono>

#include "libarchive/libarchive/archive.h"
#include "libarchive/libarchive/archive_entry.h"

void DebugPrint(const char* fmt, ...)
{
    va_list list;
    va_start(list, fmt);
    char buffer[4096];
    vsnprintf(buffer, sizeof(buffer), fmt, list);
    OutputDebugStringA(buffer);
    OutputDebugStringA("\n");
    va_end(list);

    //If we have a console, print there too
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    if (console != NULL && console != INVALID_HANDLE_VALUE)
    {
        DWORD mode;
        if (GetConsoleMode(console, &mode)) // succeeds only if console attached
        {
            DWORD written;
            WriteConsoleA(console, buffer, (DWORD)strlen(buffer), &written, NULL);
            const char* new_line = "\n";
            WriteConsoleA(console, new_line, (DWORD)strlen(new_line), &written, NULL);
        }
    }
}
void DebugPrint(const wchar_t* fmt, ...)
{
    va_list list;
    va_start(list, fmt);
    wchar_t buffer[4096];
    _vsnwprintf(buffer, sizeof(buffer), fmt, list);
    OutputDebugStringW(buffer);
    va_end(list);

    //If we have a console, print there too
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    if (console != NULL && console != INVALID_HANDLE_VALUE)
    {
        DWORD mode;
        if (GetConsoleMode(console, &mode)) // succeeds only if console attached
        {
            DWORD written;
            WriteConsoleW(console, buffer, (DWORD)wcslen(buffer), &written, NULL);
            const char* new_line = "\n";
            WriteConsoleA(console, new_line, (DWORD)strlen(new_line), &written, NULL);
        }
    }
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

i32 RunProcess(const wchar_t* path, const wchar_t* args, bool async, bool show)
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
    info.nShow = show ? SW_SHOW : SW_HIDE;
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
    if (!async)
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

i32 RunProcess(std::string& output, const wchar_t* path, const wchar_t* args)
{
#if 1
    SECURITY_ATTRIBUTES sa{ sizeof(sa) };
    sa.bInheritHandle = TRUE;

    HANDLE readPipe = nullptr;
    HANDLE writePipe = nullptr;
    CreatePipe(&readPipe, &writePipe, &sa, 0);

    // Parent should not inherit read end
    SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = writePipe;
    si.hStdError  = writePipe;
    si.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);

    PROCESS_INFORMATION pi{};

    BOOL r = CreateProcessW(
        (LPWSTR)path,
        (LPWSTR)args,
        nullptr, nullptr,
        TRUE, // inherit handles
        CREATE_NO_WINDOW,
        nullptr, nullptr,
        &si, &pi
    );

    CloseHandle(writePipe); // parent reads only

    char buffer[4096];
    DWORD bytesRead;

    while (ReadFile(readPipe, buffer, sizeof(buffer), &bytesRead, nullptr))
        output.append(buffer, bytesRead);

    CloseHandle(readPipe);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return r;
#else
    //SECURITY_DESCRIPTOR security_descriptor;
    //InitializeSecurityDescriptor(&security_descriptor, SECURITY_DESCRIPTOR_REVISION);
    //FAIL;

    //SECURITY_ATTRIBUTES security_attributes = {
    //    .nLength = sizeof(SECURITY_ATTRIBUTES),
    //    .lpSecurityDescriptor = &security_attributes,
    //    .bInheritHandle = false,
    //};

    BOOL r = CreateProcess(
    path,                   //_In_opt_ LPCWSTR lpApplicationName,
    args,                   //_Inout_opt_ LPWSTR lpCommandLine,
    NULL,//security_attributes,    //_In_opt_ LPSECURITY_ATTRIBUTES lpProcessAttributes,
    NULL,//security_attributes,    //_In_opt_ LPSECURITY_ATTRIBUTES lpThreadAttributes,
    false,//_In_ BOOL bInheritHandles,
    CREATE_NO_WINDOW,//_In_ DWORD dwCreationFlags,
    //_In_opt_ LPVOID lpEnvironment,
    //_In_opt_ LPCWSTR lpCurrentDirectory,
    //_In_ LPSTARTUPINFOW lpStartupInfo,
    //_Out_ LPPROCESS_INFORMATION lpProcessInformation
    );
#endif
}

void RunProcessJob::RunJob()
{
    const wchar_t* path = applicationPath.size()   ? applicationPath.c_str()   : nullptr;
    const wchar_t* args = arguments.size()         ? arguments.c_str()         : nullptr;
    i32 result = RunProcess(path, args);
    if (result)
    {
        Threading::GetInstance().RunAndClearJobs();
    }
}

void RunProcessLogJob::RunJob()
{
    const wchar_t* path = applicationPath.size()   ? applicationPath.c_str()   : nullptr;
    const wchar_t* args = arguments.size()         ? arguments.c_str()         : nullptr;
    i32 result = RunProcess(output, path, args);
    if (result)
    {
        Threading::GetInstance().RunAndClearJobs();
    }
}

const wchar_t* valid_file_exts[] = { L".mp4", L".mov", L".wmv", L".avil", L".mkv", L".webm" };

MATH_PREFIX int QuickSortFilenameComparisonFunction(const void* a, const void* b)
{
    const std::wstring& aws = *(std::wstring*)a;
    const std::wstring& bws = *(std::wstring*)b;
    const size_t max = Min(aws.size(), bws.size());
    for (size_t i = 0; i < max; i++)
    {
        const i32 diff = bws[i] - aws[i];
        if (diff)
        {
            return diff;
        }
    }
    return (i32)(bws.size() - aws.size());
}

template <typename T>
void AppendProperty(std::string& out, const nlohmann::json& json, const char* property_name)
{
    if (json.contains(property_name))
    {
        const T& property = json[property_name];
        out = std::format("{}[{}]", out.c_str(), property);
    }
}

//void RunUpdateVideoGroupJob::RunJob()
//{
    //VALIDATE(video_group);
    //std::vector<std::wstring> filenames;
    //ScanDirectoryForFileNames(source_path, filenames, false);
    //if (!filenames.size())
    //    return;

    ////NOTE(CSH): Windows in thier infinite wisdom does some sort of random sort SOMETIMES so we must always sort
    //QuickSort((u8*)filenames.data(), (i32)filenames.size(), sizeof(filenames[0]), QuickSortFilenameComparisonFunction);

    //std::lock_guard<std::mutex> lock(video_group->thread_lock);
    //std::string json_string;
    //for (size_t i = 0; i < filenames.size() && !video_group->stop; i++)
    //{
    //    bool found_valid_ext = false;
    //    for (i32 j = 0; j < arrsize(valid_file_exts); j++)
    //    {
    //        if (filenames[i].find(valid_file_exts[j]) != std::wstring::npos)
    //        {
    //            found_valid_ext = true;
    //            break;
    //        }
    //    }
    //    if (!found_valid_ext)
    //        continue;
    //    json_string.clear();
    //    //std::wstring full_path = source_path + filenames[i].c_str();
    //    //std::wstring args = ToString(L"-J \"%s\"", full_path.c_str());
    //    //std::wstring full = mkv_path + L" " + args;
    //    std::wstring full = std::format(L"{} -J \"{}{}\"", mkv_path.c_str(), source_path.c_str(), filenames[i].c_str());

    //    i32 r = RunProcess(json_string, nullptr, full.c_str());
    //    nlohmann::json data;
    //    if (r)
    //    {
    //        data = nlohmann::json::parse(json_string);
    //    }
    //    if (r && !data["errors"].size())
    //    {
    //        nlohmann::json data = nlohmann::json::parse(json_string);
    //        video_group->video_infos.push_back({});
    //        VideoInfo* video_info = &video_group->video_infos[video_group->video_infos.size() - 1];
    //        video_info->name = filenames[i];
    //        if (data.contains("tracks"))
    //        {
    //            ASSERT(data["tracks"].is_array());
    //            const auto& tracks = data["tracks"];
    //            video_group->max_tracks = Max((i32)tracks.size(), video_group->max_tracks);
    //            bool already_have_video = false;
    //            bool already_have_audio = false;
    //            bool already_have_sub = false;
    //            for (size_t t = 0; t < tracks.size(); t++)
    //            {
    //                video_info->tracks.push_back({});
    //                Track* track = &video_info->tracks[video_info->tracks.size() - 1];
    //                const auto& jt = tracks[t];
    //                if (jt["id"] != t)
    //                {
    //                    FAIL;
    //                }
    //                if (!jt.contains("type") || !jt.contains("properties") || !jt.contains("id"))
    //                    continue;
    //                const auto& type = jt["type"];
    //                const auto& props = jt["properties"];
    //                track->type = type;
    //                track->id = jt["id"];
    //                if (type == "video")
    //                {
    //                    AppendProperty<std::string>(track->details, props, "codec");
    //                    AppendProperty<std::string>(track->details, props, "display_dimensions");
    //                    if (!already_have_video)
    //                    {
    //                        track->encode = true;
    //                        already_have_video = true;
    //                    }
    //                }
    //                else if (type == "audio")
    //                {
    //                    track->type = type;
    //                    AppendProperty<std::string> (track->details, props, "language");
    //                    AppendProperty<int>         (track->details, props, "audio_channels");
    //                    AppendProperty<std::string> (track->details, props, "codec");

    //                    if (!already_have_audio)
    //                    {
    //                        track->encode = true;
    //                        already_have_audio = true;
    //                    }
    //                }

    //                else if (type == "subtitles")
    //                {
    //                    track->type = "sub";
    //                    AppendProperty<std::string>(track->details, props, "language");
    //                    AppendProperty<std::string>(track->details, props, "track_name");
    //                    AppendProperty<std::string>(track->details, props, "codec_id");

    //                    if (!already_have_sub)
    //                    {
    //                        track->encode = true;
    //                        already_have_sub = true;
    //                    }
    //                }
    //            }
    //        }
    //    }
    //    else
    //    {
    //        video_group->video_infos.push_back({});
    //        VideoInfo* video_info = &video_group->video_infos[video_group->video_infos.size() - 1];
    //        video_group->max_tracks = Max(1, video_group->max_tracks);
    //        std::string error = "unknown";
    //        if (data["errors"].size())
    //            error = data["errors"][0];
    //        std::wstring werror;
    //        ConvertMultibyteToWideChar(werror, error);
    //        video_info->name = std::format(L"FAILED {}: {}", i + 1, werror);
    //    }
    //}
//}

void RunEncodeJob::RunJob()
{
    //VALIDATE(video_group);
    //VALIDATE(mkv_path.size());
    //VALIDATE(source_path.size());
    //VALIDATE(video_group->video_infos.size());
    //video_group->in_progress = true;
    //Defer{ video_group->in_progress = false; };
    //std::wstring dest_folder = dest_path;
    //if (!dest_folder.size())
    //    dest_folder = ToString(L"%sencoded/", source_path.c_str());

    //std::wstring path = mkv_path;
    //for (size_t v = 0; v < video_group->video_infos.size() && !video_group->stop; v++)
    //{
    //    const VideoInfo& video = video_group->video_infos[v];
    //    if (!video.encode)
    //    {
    //        video_group->completed++;
    //        continue;
    //    }
    //    const std::wstring full_src = source_path.c_str() + video.name;
    //    const std::wstring full_dst = dest_folder.c_str() + video.name;

    //    std::wstring audio;
    //    std::wstring subs;
    //    for (size_t i = 0; i < video.tracks.size(); i++)
    //    {
    //        const Track& track = video.tracks[i];
    //        if (!track.encode)
    //            continue;
    //        if (track.type == "audio")
    //        {
    //            if (!audio.size())
    //                audio = ToString(L"%i", track.id);
    //            else
    //                audio = ToString(L"%s,%i", audio.c_str(), track.id);
    //        }
    //        else if (track.type == "sub")
    //        {
    //            if (!subs.size())
    //                subs = ToString(L"%i", track.id);
    //            else
    //                subs = ToString(L"%s,%i", subs.c_str(), track.id);
    //        }
    //    }

    //    std::wstring args = ToString(L"--audio-tracks \"%s\" --subtitle-tracks \"%s\" -m \"%s\",\"%s\" -o \"%s\" \"%s\"",
    //        audio.c_str(), subs.c_str(), audio.c_str(), subs.c_str(), full_dst.c_str(), full_src.c_str());
    //    i32 result = RunProcess(path.c_str(), args.c_str(), false, false);
    //    if (result > 0)
    //    {
    //        FAIL;
    //    }
    //    video_group->completed++;
    //}
    //NotifyWindowBuildFinished();
    //video_group->completed = 0;
    //video_group->in_progress = false;
}

HMODULE modh;
HWND window_handle;

void InitOS(GLFWwindow* window)
{
    modh = GetModuleHandle(NULL);
    VALIDATE(modh != NULL);

    window_handle = glfwGetWin32Window(window);
    VALIDATE(window_handle);
#if 1
    HRSRC res = FindResource(nullptr, MAKEINTRESOURCE(IDB_PNG1), RT_RCDATA);
    DWORD error = GetLastError();
    VALIDATE(res);
    HGLOBAL resh = LoadResource(nullptr, res);
    VALIDATE(resh);
    DWORD size = SizeofResource(nullptr, res);
    void* data = LockResource(resh);
    VALIDATE(data);

    // ---- Decode PNG from memory ----
    GLFWimage image{};
    image.pixels = stbi_load_from_memory(
        (const stbi_uc*)data,
        size,
        &image.width,
        &image.height,
        nullptr,
        4
    );
    VALIDATE(image.pixels);

    glfwSetWindowIcon(window, 1, &image);
    stbi_image_free(image.pixels);
#else
    GLFWimage images[1] = {};
    images[0].pixels = stbi_load("assets/QuantumFullSize.png",  &images[0].width, &images[0].height, 0, 4);
    //images[1].pixels = stbi_load("assets/QuantumIcon.ico",      &images[1].width, &images[1].height, 0, 4);
    glfwSetWindowIcon(window, arrsize(images), images);
    for (i32 i = 0; i < arrsize(images); i++)
    {
        stbi_image_free(images[i].pixels);
    }
#endif
}

bool ConsoleAttached()
{
    return AttachConsole(ATTACH_PARENT_PROCESS);
}
bool DebuggerAttached()
{
    return IsDebuggerPresent();
}

class DebugStreamBuffer final : public std::streambuf
{
protected:
    int overflow(int c) override
    {
        if (c != EOF)
        {
            OutputDebugStringA((char*)&c);
        }
        return c;
    }
};
static DebugStreamBuffer g_debug_stream_buffer;
void EnableOutputToDebugger()
{
    std::cerr.rdbuf(&g_debug_stream_buffer);
    std::cout.rdbuf(&g_debug_stream_buffer);
}

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

void Sleep(u64 _ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(_ms));
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
    FAIL;
    //int msgboxID = MessageBox(
    //    NULL,
    //    text.c_str(),
    //    title.c_str(),
    //    MB_ABORTRETRYIGNORE | MB_ICONSTOP | MB_DEFBUTTON1 | MB_APPLMODAL
    //);

    //switch (msgboxID)
    //{
    //case IDABORT:
    //    SDL_Event e;
    //    e.type = SDL_QUIT;
    //    e.quit.timestamp = 0;
    //    SDL_PushEvent(&e);
    //    break;
    //case IDRETRY:
    //    break;
    //case IDIGNORE:
    //    break;
    //}
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

void NotifyWindowBuildFinished()
{
    FLASHWINFO info = {};
    info.hwnd = window_handle;
    info.dwFlags = FLASHW_TRAY | FLASHW_TIMERNOFG;
    info.uCount;
    info.dwTimeout;
    info.cbSize = sizeof(info);

    FlashWindowEx(&info);
}

//TODO(CSH): Create filepath helper functions
void _ScanDirectoryForFileNames(const std::wstring& root, const std::wstring& dir, std::vector<ScannedFile>& out, ScanDirectoryFlags flags)
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

void ScanDirectoryForFileNames(const std::wstring& dir, std::vector<ScannedFile>& out, ScanDirectoryFlags flags)
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
        .hwndOwner = window_handle,
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
    SRWLock() { InitializeSRWLock(&lock); }
    SRWLOCK lock;
};

static SRWLock s_assert_mutex;
static std::vector<AssertRecord> s_assert_records;
void OsAssert(bool expr, const char* message, const char* file, int line)
{
    if (!expr)
    {
        AcquireSRWLockExclusive(&s_assert_mutex.lock);
        Defer { ReleaseSRWLockExclusive(&s_assert_mutex.lock); };

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
        sprintf_s(info_buffer_view.data, (size_t)info_buffer_view.Bytes(), "%s(%d)\n\n"
                                                     "This has been hit %d time%s.\n\n"
                                                     "Yes   : Break into debugger\n"
                                                     "No    : Continue execution\n"
                                                     "Retry : Ignore this assert in the future\n"
                                                     "Close : Abort the program",
                                                     file, line, record->hit_counter, s);
        WCHAR winfo[1024];
        auto winfo_view = CreateArrayView(winfo);
        winfo_view.Last() = 0;
        if (MultiByteToWideChar(CP_UTF8, 0, info_buffer, -1, winfo_view.data, (int)winfo_view.Bytes()) == 0)
        {
            wcscpy_s(winfo, (size_t)winfo_view.Bytes(), L"Error");
        }


        int button = 0;
        TaskDialog(NULL, NULL,
                   L"Assertion Failed",
                   wmessage,
                   winfo,
                   TDCBF_YES_BUTTON | TDCBF_NO_BUTTON | TDCBF_RETRY_BUTTON | TDCBF_CLOSE_BUTTON,
                   TD_WARNING_ICON,
                   &button);

        switch(button)
        {
        default:
        case IDNO:
        case IDCANCEL: {
        } break;

        case IDYES: {
            __debugbreak();
        } break;

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

int main(int argc, char** argv)
{
    return Main(argc, argv);
}
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR str, int val)
{
    int argc = 0;

    LPWSTR* argv_w = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv_w)
        return -1;

    // Convert to UTF-8 if your Main expects char**
    std::vector<std::string> argv_utf8;
    std::vector<char*> argv;

    char exe_path[MAX_PATH];
    GetModuleFileNameA(nullptr, exe_path, MAX_PATH);
    argv.push_back(exe_path);

    for (int i = 0; i < argc; ++i)
    {
        int size = WideCharToMultiByte(
            CP_UTF8, 0,
            argv_w[i], -1,
            nullptr, 0,
            nullptr, nullptr
        );

        argv_utf8.emplace_back(size, '\0');
        WideCharToMultiByte(
            CP_UTF8, 0,
            argv_w[i], -1,
            argv_utf8.back().data(), size,
            nullptr, nullptr
        );

        argv.push_back(argv_utf8.back().data());
    }

    LocalFree(argv_w);

    return Main((int)argv.size(), argv.data());
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

void AddEntryToZip(archive* a, const std::filesystem::path& full_path, const std::filesystem::path& relative_path, bool is_dir, std::vector<u8>& file_buffer)
{
    //const std::filesystem::path relative = relative_path_to_file / f.name;
    //const std::filesystem::path fullpath = source / relative;
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
            /* Should never be reached. */
            DebugPrint("Unexpected error in _stat.\n");
        }
        FAIL;
        return;
    }
    if (is_dir)
    {
        //archive_entry_set_pathname(entry,  relative_path.string().c_str());
        //archive_entry_set_filetype(entry, AE_IFDIR); //Directory
        //archive_entry_copy_stat(entry, &st);
        //int error = archive_write_header(a, entry);
        //ArchiveErrorCheck(a, error);
        //++g_data.progress;

        std::vector<ScannedFile> out;
        ScanDirectoryForFileNames(full_path, out, ScanDirectoryFlags_IncludeDirs);
        for (i32 i = 0; i < out.size(); i++)
        {
            AddEntryToZip(a, full_path / out[i].name, relative_path / out[i].name, out[i].dir, file_buffer);
        }
    }
    else
    {
        archive_entry* entry = archive_entry_new();
        archive_entry_set_pathname(entry, relative_path.string().c_str());
        archive_entry_set_filetype(entry, AE_IFREG); //Regular file
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
            const size_t file_size = file.tellg();
            if (file_size > file_buffer.size())
                file_buffer.resize(file_size * 2);
            file.seekg(0, std::ios::beg);
            file.read((char*)file_buffer.data(), file_size);

            error = (int)archive_write_data(a, file_buffer.data(), file_size);
            if (error < 0)
                ArchiveErrorCheck(a, error);
            ++g_data.progress;
        }
        archive_entry_free(entry);
    }
}

void CreateZip(const std::wstring& zip_name, const std::wstring& zip_pathw, const std::wstring& source_folder, ArrayView<ScannedFile> files_to_backup, ArrayView<std::filesystem::path> files_to_add_to_root/*, ArrayView<std::wstring> ext_to_exclude*/)
{
    archive* a = archive_write_new();
    archive_write_set_format_zip(a);
    int error = archive_write_zip_set_compression_deflate(a);
    ArchiveErrorCheck(a, error);
    error = archive_write_set_options(a, "compression-level=9");
    ArchiveErrorCheck(a, error);
    std::filesystem::path zip_filename = std::filesystem::path(zip_pathw) / zip_name;
    error = archive_write_open_filename(a, zip_filename.string().c_str());
    ArchiveErrorCheck(a, error);

    std::vector<u8> file_buffer;
    //file_buffer.reserve(64*1000*1000);
    std::filesystem::path source = source_folder;
    for (i32 i = 0; i < files_to_backup.size(); i++)
    {
        std::filesystem::path full = source / files_to_backup[i].name;
        AddEntryToZip(a, full, files_to_backup[i].name, files_to_backup[i].dir, file_buffer);
    }
    for (i32 i = 0; i < files_to_add_to_root.size(); i++)
    {
        if (files_to_add_to_root[i].extension() != ".ini")
            continue;
        AddEntryToZip(a, files_to_add_to_root[i], files_to_add_to_root[i].filename(), false, file_buffer);
    }

    error = archive_write_close(a);
    ArchiveErrorCheck(a, error);
    error = archive_write_free(a);
    ArchiveErrorCheck(a, error);
}

ImFont* LoadFontForImgui(int resource_id, float fontSize)
{
    HRSRC r = FindResource( nullptr, MAKEINTRESOURCE(resource_id), RT_RCDATA);
    if (!r)
        return nullptr;

    HGLOBAL handle = LoadResource(nullptr, r);
    if (!handle)
        return nullptr;
    DWORD size = SizeofResource(nullptr, r);
    void* data = LockResource(handle);
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