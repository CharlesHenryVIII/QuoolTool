#pragma once
#include "Math.h"
#include "Themes.h"
#include "String.h"
#include "Threading.h"


struct CitectData {
    Path project_path;
    Path program_files_path;
    Path program_files_86;
    Path backup_path;
    Mutex lock;
};

struct Settings {
    ThemeColor color = ThemeColor_Quantum;
    ThemeStyle style = ThemeStyle_SimpleRounding;
};

struct GlobalData
{
    CitectData citect = {};
    Settings settings;
    std::atomic<bool> backup_in_progress = false;
    std::atomic<u64> progress;
    std::atomic<u64> total;
};

extern GlobalData g_data;
