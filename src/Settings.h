#pragma once
#include "Math.h"
#include "Themes.h"
#include "String.h"
#include "Threading.h"

struct ToolsData {
    Path output_path;
    Atomic<bool> running = false;
    Atomic<u64> progress = 0;
    Atomic<u64> total = 0;
    TRACY_MUTEX(lock);
};

struct CitectData {
    Path project_path;
    Path program_files_path;
    Path program_files_86;
    Path backup_path;
    TRACY_MUTEX(lock);
    Atomic<bool> backup_in_progress = false;
    Atomic<u64> progress;
    Atomic<u64> total;
};

struct AppData {
    ToolsData tools_data;
    CitectData citect_data;
};

struct Settings {
    ThemeColor color = ThemeColor_Quantum;
    ThemeStyle style = ThemeStyle_SimpleRounding;
};

enum FontIndex : u32 {
    FontIndex_Default,
    FontIndex_Monospace,
    FontIndex_Count,
};

struct ImFont;
struct GlobalData
{
    Settings settings;
    ImFont* fonts[FontIndex_Count] = {};
};

extern GlobalData g_data;
