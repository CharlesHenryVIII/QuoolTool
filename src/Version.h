#pragma once
#include "Math.h"

struct Version
{
    u32 major = 0;
    u32 minor = 0;
    std::string AsString() const;
    bool IsValid() const;
    //format is "v1.2"
    void SetFromTag(const std::string& tag);
};
extern Version g_version;
