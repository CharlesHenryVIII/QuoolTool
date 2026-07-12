#pragma once
#include "Version.h"
#include "CashUtil\CashUtil.h"
#include <vector>

extern Version g_online_version;
extern Atomic<AsyncStatus> g_version_state;
extern Atomic<AsyncStatus> g_download_state;
extern Atomic<float> g_download_update_progress;

struct NetworkSettings {
    std::vector<SysNetAdapterConfig> configs;
};
extern NetworkSettings g_network_settings;

struct NetworkData {
    AsyncData<std::vector<SysNetworkAdapterInfo>> adapters;
    bool is_admin;
};

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

void NetworkingInit(NetworkData** network_data);
void NetworkingDestroy(NetworkData** network_data);
void NetworkImgui(NetworkData& data);
