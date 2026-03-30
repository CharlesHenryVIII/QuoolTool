#pragma once
#include "Threading.h"
#include "imgui.h"
#include "Math.h"
#include "ArrayView.h"
#include "Settings.h"
#include "String.h"


void OSDebugOutput(const char* s);
void OSDebugOutput(const wchar_t* s);
i32 OSRunShellProcess(const wchar_t* path, const wchar_t* args, std::string* output = nullptr, Mutex* output_lock = nullptr, RunProcessFlags flags = RunProcess_None);
i32 OSRunProcess(const char*         path, const char*         args, AsyncData<std::string>* output = nullptr, AsyncData<Path>* output_file = nullptr, RunProcessFlags flags = RunProcess_None);
i32 OSRunProcess(const wchar_t*      path, const wchar_t*      args, AsyncData<std::string>* output = nullptr, AsyncData<Path>* output_file = nullptr, RunProcessFlags flags = RunProcess_None);
i32 OSRunProcess(const std::string&  path, const std::string&  args, AsyncData<std::string>* output = nullptr, AsyncData<Path>* output_file = nullptr, RunProcessFlags flags = RunProcess_None);
i32 OSRunProcess(const std::wstring& path, const std::wstring& args, AsyncData<std::string>* output = nullptr, AsyncData<Path>* output_file = nullptr, RunProcessFlags flags = RunProcess_None);
struct SDL_Window;
bool OSInit(SDL_Window* window);
void OSDestroy(SDL_Window* window);
void* OSGetWindowHandle(SDL_Window* window);
bool OSGetNetworkAdapters(std::vector<OSNetworkAdapterInfo>& out_adapters);

bool OSConsoleAttached();
bool OSDebuggerAttached();
void OSHideConsole();
void OSShowConsole();
bool OSIsConsoleVisible();

static bool keepOpen = true;
void OSShowErrorWindow(const std::wstring& title, const std::wstring& text);
void OSFlashWindow(SDL_Window* window);
void OSScanDirectoryForFileNames(const std::wstring& dir, ScannedFiles& out, ScanDirectoryFlags flags);
bool OSGetDirectoryFromUser(const std::wstring& currentDir, std::wstring& dir);

void OSConvertMultibyteToWideChar(std::wstring& out, const std::string& in);
void OSConvertWideCharToMultiByte(std::string& out, const std::wstring& in);
void OSExpandEnvironemntVariable(std::wstring& out, const std::wstring& in);
void* OsGetDataFromResource(i32* out_size, const i32 resource_id);

Guid OSNewGuid();