#pragma once
#include "Math.h"

struct Version
{
    u32 major;
    u32 minor;
    std::string AsString() const;
};
extern Version g_version;
