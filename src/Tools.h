#pragma once
#include "Settings.h"

enum ScriptInfoFlags : u32 {
    ScriptInfoFlags_None            = 0,
    ScriptInfoFlags_Enabled         = BIT(0),
    //ScriptInfoFlags_Completed       = BIT(1),
};
ENUMOPS_PURE(ScriptInfoFlags);

void ToolsImGui(ToolsData& td);
