#pragma once
#include "CashUtil\CashUtil.h"

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
    ImFont* fonts[FontIndex_Count] = {};
};

extern GlobalData g_data;
