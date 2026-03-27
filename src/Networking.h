#pragma once
#include "String.h"
#include "Version.h"
#include "Threading.h"

extern Version g_online_version;
extern Atomic<AsyncStatus> g_version_state;
extern Atomic<AsyncStatus> g_download_state;
extern Atomic<float> g_download_update_progress;

//union IPAddress {
//    struct { u8 a,b,c,d; };
//    u8 e[4];
//    u64 addr;
//    IPAddress() = default;
//    IPAddress(u8 v) : a(v), b(v), c(v), d(v) {};
//};

struct EnvironmentVariables
{
    std::string github_api_key;
};

struct DownloadUpdateJob : Job
{
    virtual void RunJob() override;
};

struct GetOnlineVersionJob : Job
{
    virtual void RunJob() override;
};

void NetworkingInit();