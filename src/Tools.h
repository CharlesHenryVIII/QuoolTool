#pragma once
#include "Threading.h"
#include "String.h"

enum ScriptInfoFlags : u32 {
    ScriptInfoFlags_None            = 0,
    ScriptInfoFlags_Enabled         = BIT(0),
    //ScriptInfoFlags_Completed       = BIT(1),
};
ENUMOPS_PURE(ScriptInfoFlags);

enum ScriptState : u32 {
    ScriptState_Standby,
    ScriptState_Scripts,
    ScriptState_Workbook,
    ScriptState_Finished,
};
ENUMOPS_PURE(ScriptState);

struct ToolsData {
    Path output_path;
    std::wstring computer_name;
    Atomic<ScriptState> state = {};
    Atomic<u64> progress = 0;
    Atomic<u64> total = 0;
    TRACY_MUTEX(lock);
};

void ToolsImGui(ToolsData& td);
