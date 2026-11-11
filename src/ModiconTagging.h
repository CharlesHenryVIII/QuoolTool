#pragma once
#include "CashUtil/CashUtil.h"

enum TagType {
    TagType_BOOL,
    TagType_BYTE,
    TagType_DATE,
    TagType_DINT,
    TagType_DT,
    TagType_DWORD,
    TagType_EBOOL,
    TagType_INT,
    TagType_REAL,
    TagType_STRING,
    TagType_TIME,
    TagType_TOD,
    TagType_UDINT,
    TagType_UINT,
    TagType_WORD,
    TagType_Count,
};

enum ModbusType {
    ModbusType_M,
    ModbusType_MW,
    ModbusType_Count,
};

struct ModiconData {
    i32 starting_addresses[ModbusType_Count];
    Path file;
};

void ModiconInit(ModiconData** data);
void ModiconDestroy(ModiconData** data);
void ModiconImGui(ModiconData& data);

ModbusType GetModbusType(TagType type);
i32 GetModbusSize(TagType type);


extern const char* g_TagTypeNames[TagType_Count];
extern const char* g_modbus_prefixes[ModbusType_Count];
