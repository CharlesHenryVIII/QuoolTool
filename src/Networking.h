#pragma once
#include "String.h"
#include "Version.h"
#include "Threading.h"

enum DownloadState : u32 {
    DownloadState_Empty = 0,
    DownloadState_Fetching = BIT(1),
    DownloadState_Fetched = BIT(2),
    DownloadState_Count = 3,
};
ENUMOPS_PURE(DownloadState);

extern Version g_online_version;
extern Atomic<DownloadState> g_version_state;
extern Atomic<DownloadState> g_download_state;
extern Atomic<float> g_download_update_progress;

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