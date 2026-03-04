#include "Version.h"
#include "WinInterop.h"

Version g_version = { .major = 1, .minor = 0 };

std::string Version::AsString() const
{
    std::string r = ToString("v%i.%i", major, minor);
    return r;
}


void CheckForUpdate()
{

}
