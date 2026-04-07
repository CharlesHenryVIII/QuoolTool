#pragma once
#include "String.h"
#include "Version.h"
#include "Threading.h"
#include "System.h"

#include <vector>

extern Version g_online_version;
extern Atomic<AsyncStatus> g_version_state;
extern Atomic<AsyncStatus> g_download_state;
extern Atomic<float> g_download_update_progress;

struct NetAdapterConfig {
    SysIP4AndSubnet ip;
    SysIP4 gateway;
    SysIP4 dns1;
    SysIP4 dns2;
    bool dhcp_enabled; //DHCP Enabled
    bool ddns_enabled; //Dynamic DNS Enabled
};

struct NetworkSettings {
    std::vector<SysIP4> ips;
};

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
void NetworkImGui(NetworkData& data);
