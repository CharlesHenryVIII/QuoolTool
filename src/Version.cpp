#include "Version.h"
#include "CashUtil.h"

Version g_version = { .major = 1, .minor = 9 };

const char* Version::AsTagString(Arena* arena) const
{
    const char* r = ArenaPush(arena, "v%i.%i", major, minor);
    return r;
}

const char* Version::AsString(Arena* arena) const
{
    const char* r = ArenaPush(arena, "%i.%i", major, minor);
    return r;
}

bool Version::IsValid() const
{
    return major != 0;
}

void Version::SetFromTag(const char* tag)
{
	const char* start = (tag + 1);
	const char* p = StringContains(tag, '.');
	VALIDATE_M(p, LogLevel_Error, "Error: Invalid tag for Version: %s", tag);
	p++;
	major = strtol(start, nullptr, 10);
	minor = strtol(p, nullptr, 10);
}

