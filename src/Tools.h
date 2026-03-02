#pragma once
#include "Settings.h"

enum ScriptInfoFlags : u32 {
    ScriptInfoFlags_None            = 0,
    ScriptInfoFlags_Enabled         = BIT(0),
    ScriptInfoFlags_ManualOutput    = BIT(1),
};

void ToolsImGui(ToolsData& td);
