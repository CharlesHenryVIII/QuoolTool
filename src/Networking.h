#pragma once
#include "String.h"
#include "Version.h"
#include "Threading.h"

extern Version g_online_version;
extern Atomic<bool> g_fetching_version;
extern Atomic<bool> g_fetching_download;

struct EnvironmentVariables
{
    std::string github_api_key;
    std::string github_pat;
};

struct GetOnlineVersionJob : Job
{
    virtual void RunJob() override;
};

struct DownloadUpdateJob : Job
{
    virtual void RunJob() override;
};

void NetworkingInit();