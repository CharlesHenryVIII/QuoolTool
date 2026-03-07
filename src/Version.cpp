#include "Version.h"
#include "WinInterop.h"

Version g_version = { .major = 1, .minor = 3 };

std::string Version::AsTagString() const
{
    std::string r = ToString("v%i.%i", major, minor);
    return r;
}

std::string Version::AsString() const
{
    std::string r = ToString("%i.%i", major, minor);
    return r;
}

bool Version::IsValid() const
{
    return major != 0;
}

void Version::SetFromTag(const std::string& tag)
{
    std::string full = tag.substr(1, tag.size());
    size_t p_loc = full.find_first_of('.');
    if (p_loc == std::string::npos)
    {
        DebugPrint("Error: Invalid tag for Version: %s", tag.c_str());
        FAIL;
        return;
    }

    const std::string mjs = full.substr(0, p_loc);
    const std::string mms = full.substr(p_loc + 1, full.size());
    major = atoi(mjs.c_str());
    minor = atoi(mms.c_str());
}

