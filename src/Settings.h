#pragma once
#include "Math.h"
#include "Themes.h"


struct Settings {
    ThemeColor color = ThemeColor_Quantum;
    ThemeStyle style = ThemeStyle_SimpleRounding;
};

enum FontIndex : u32 {
    FontIndex_Default,
    FontIndex_Small,
    FontIndex_Imgui,
    FontIndex_Monospace,
    FontIndex_Count,
};

struct DataCollectionData;
struct CitectData;
struct NetworkData;
struct AppData {
    DataCollectionData* data_collection_data = nullptr;
    CitectData* citect_data = nullptr;
    NetworkData* network_data = nullptr;
};

struct ImFont;
struct GlobalData
{
    Settings settings;
    ImFont* fonts[FontIndex_Count] = {};
};

extern GlobalData g_data;
