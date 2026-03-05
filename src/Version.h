#pragma once
#include "Math.h"

struct Version
{
    u32 major = 0;
    u32 minor = 0;
    //format is "v1.2"
    std::string AsTagString() const;
    //format is "1.2"
    std::string AsString() const;
    bool IsValid() const;
    //format is "v1.2"
    void SetFromTag(const std::string& tag);
};
inline bool operator>(Version a, Version b) { return (a.major > b.major) || ((a.major == b.major) && (a.minor > b.minor)); };
extern Version g_version;
