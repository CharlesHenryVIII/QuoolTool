#include "curl/curl.h"

#include "Networking.h"
#include "LoadJson.h"
#include "Archive.h"
#include "ImguiHelper.h"

#include "json.hpp"

#include <fstream>
#include <iostream>

using Json = nlohmann::json;

struct NetworkInfo {
    const std::string url = "https://api.github.com/repos/CharlesHenryVIII/QuoolTool/releases/latest";
    const std::wstring env_filename = L".env";
    EnvironmentVariables env;
    std::string download_url;
    size_t download_size;
};
NetworkInfo s_network;
Version g_online_version = {};
Atomic<AsyncStatus> g_version_state;
Atomic<AsyncStatus> g_download_state;
Atomic<float> g_download_update_progress = 0;
NetworkSettings g_network_settings;

std::string GetUrlFromVersion(Version v)
{
    std::string r = ToString("https://github.com/CharlesHenryVIII/QuoolTool/releases/download/%s/QuoolTool_windows_x64_Release.zip", v.AsTagString().c_str());
                            //https://github.com/CharlesHenryVIII/QuoolTool/releases/download/v1.1/QuoolTool_windows_x64_Release.zip
    return r;
}

template<typename T>
struct ResponseData {
    T data;
    Atomic<float>* progress = nullptr;
    Atomic<size_t> completed = 0;
    size_t total;
};

static size_t WriteCallbackString(void* contents, size_t size, size_t nmemb, std::string* out)
{
    ASSERT(size == 1);
    out->append((char*)contents, size * nmemb);
    return size * nmemb;
}
static size_t WriteCallbackBinary(void* contents, size_t size, size_t nmemb, void* data)
{
    ResponseData<std::vector<char>>* user_data = (ResponseData<std::vector<char>>*)data;
    ASSERT(size == 1);
    VALIDATE_V(user_data, 0);
    for (size_t i = 0; i < nmemb; i++)
    {
        user_data->data.push_back(((u8*)contents)[i]);
    }
    if (user_data->progress)
    {
        user_data->completed += nmemb;
        const float progress = (float)user_data->completed / (float)user_data->total;
        ASSERT(progress > *user_data->progress);
        *user_data->progress = progress;
    }
    return nmemb;
}

#define CURLCHECK(fun)  \
{\
    CURLcode result = fun;\
    if (result != CURLE_OK)\
    {\
        DebugPrint("Error: \"%s\" failed at %s(%i) CURLcode: %i ", #fun, __FILENAME__, __LINE__, result);\
    }\
} REQUIRE_SEMICOLON

void DownloadUpdateJob::RunJob()
{
    ZoneScopedN("NetworkingJob: DownloadUpdateJob");
    g_download_state = AsyncStatus_Fetching;
    if (!s_network.download_url.size())
    {
        FAIL;
        g_download_state = AsyncStatus_FetchedFailed;
        return;
    }

    CURL* curl = curl_easy_init();
    struct curl_slist* headers = nullptr;
    if (s_network.env.github_api_key.size() > 10)
    {
        const std::string auth = "Authorization: Bearer " + s_network.env.github_api_key;
        headers = curl_slist_append(headers, auth.c_str());
    }
    headers = curl_slist_append(headers, "Accept: application/octet-stream");
    headers = curl_slist_append(headers, "X-GitHub-Api-Version: 2022-11-28");
    CURLCHECK(curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers));

    ResponseData<std::vector<char>> response = {
        .progress = &g_download_update_progress,
        .total = s_network.download_size };
    std::string url = s_network.download_url;
    CURLCHECK(curl_easy_setopt(curl, CURLOPT_URL, url.c_str()));
    CURLCHECK(curl_easy_setopt(curl, CURLOPT_USERAGENT, "QuoolToolUpdater"));
    CURLCHECK(curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallbackBinary));
    CURLCHECK(curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response));
    CURLCHECK(curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L));

    CURLCHECK(curl_easy_perform(curl));

    if (s_network.env.github_api_key.size() > 10)
    {
        curl_slist_free_all(headers);
    }
    curl_easy_cleanup(curl);
    g_download_update_progress = -1.0f;
    std::string zip_filename = ToString("QuoolTool_v%i_%i.zip", g_online_version.major, g_online_version.minor);
    if (response.data.size() > Megabytes(1))
    {
        std::fstream file(zip_filename, std::ios_base::out | std::ios_base::binary);
        if (!file.good())
        {
            DebugPrint("Failed to open file for write: %s", zip_filename.c_str());
            FAIL;
            g_download_state = AsyncStatus_FetchedFailed;
            return;
        }
        else
        {
            file.write((char*)response.data.data(), response.data.size());
        }
    }
    else
    {
        DebugPrint("Failed to get file from github");
        FAIL;
        g_download_state = AsyncStatus_FetchedFailed;
        return;
    }

    std::vector<std::string> filenames;
    UnzipArchive(zip_filename, "", filenames);
    for (const auto& f : filenames)
    {
        if (f.find("QuoolTool") != std::string::npos)
        {
            Path fe = zip_filename;
            std::error_code ec;
            fs::remove(zip_filename, ec);
            if (ec)
            {
                DebugPrint("Error: failed to remove file: \"%s\"", zip_filename.c_str());
                DebugPrint("\"remove\" failure: \"%d\", \"%s\"", ec.value(), ec.message().c_str());
                FAIL;
                g_download_state = AsyncStatus_FetchedFailed;
                return;
            }
        }
    }

    g_download_state = AsyncStatus_FetchedSuccess;
}

void GetOnlineVersionJob::RunJob()
{
    ZoneScopedN("NetworkingJob: GetOnlineVersionJob");
    g_version_state = AsyncStatus_Fetching;

    CURL* curl = curl_easy_init();
    struct curl_slist* headers = nullptr;
    if (s_network.env.github_api_key.size() > 10)
    {
        std::string auth = "Authorization: Bearer " + s_network.env.github_api_key;
        headers = curl_slist_append(headers, "Accept: application/vnd.github+json");
        headers = curl_slist_append(headers, auth.c_str());
        headers = curl_slist_append(headers, "X-GitHub-Api-Version: 2022-11-28");
        CURLCHECK(curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers));
    }

    std::string response;
    CURLCHECK(curl_easy_setopt(curl, CURLOPT_URL, s_network.url.c_str()));
    CURLCHECK(curl_easy_setopt(curl, CURLOPT_USERAGENT, "QuoolToolUpdater"));
    CURLCHECK(curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallbackString));
    CURLCHECK(curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response));
    CURLCHECK(curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L));

    CURLCHECK(curl_easy_perform(curl));

    if (s_network.env.github_api_key.size() > 10)
    {
        curl_slist_free_all(headers);
    }
    curl_easy_cleanup(curl);

    std::string tag;
    Json json = Json::parse(response);
    if (!JsonSafeGet(tag, &json, "tag_name"))
    {
        DebugPrint("Error: failed to get tag_name, url: %s", s_network.url.c_str());
        DebugPrint("    json response vvvvvv");
        DebugPrint("%s", response.c_str());
        g_version_state = AsyncStatus_FetchedFailed;
        return;
    }

    g_online_version.SetFromTag(tag);
    if (json.contains("assets") &&
        json["assets"].is_array() &&
        json["assets"].size() &&
        json["assets"][0].contains("url"))
    {
        const auto& asset = json["assets"][0];
        s_network.download_url = asset["url"];
        s_network.download_size = asset["size"];
    }
    g_version_state = AsyncStatus_FetchedSuccess;
}

struct MainAdapterInfo {
    std::string desc;
    std::string guid;
    SysNetAdapterConfig config;
};
static std::vector<SysNetAdapterConfig> s_current_adapter_configs;
static std::vector<MainAdapterInfo> s_modified_adapters;

void UpdateNetworkAdaptersInfo(NetworkData* nd)
{
    ZoneScoped;
    TRACY_LOCK(nd->adapters.lock);
    if (nd->adapters.state == AsyncStatus_Empty)
    {
        nd->adapters.state = SysGetNetworkAdapters(nd->adapters.data) ? AsyncStatus_FetchedSuccess : AsyncStatus_FetchedFailed;
    }
    s_current_adapter_configs.clear();
    s_modified_adapters.clear();
    for (i32 i = 0; i < nd->adapters.data.size(); i++)
    {
        const SysNetworkAdapterInfo& a = nd->adapters.data[i];
        MainAdapterInfo c;
        if (a.ipv4_ips.size() > 1)
        {
            DebugPrint("Warning: Network adapter '%s' has multiple IPv4 addresses:", c.config.name.c_str());
            for (const auto& ip : a.ipv4_ips)
            {
                DebugPrint("    %s", ip.ip.ToString().c_str());
            }
            DebugPrint("    Selecting: '%s'", a.ipv4_ips.back().ip.ToString().c_str());
        }
        SysConvertWideCharToMultiByte(c.config.name, a.friendly_name);
        SysConvertWideCharToMultiByte(c.desc, a.description);
        c.guid = a.name;
        c.config.ip = a.ipv4_ips.size() > 0 ? a.ipv4_ips.back() : SysIP4AndSubnet();
        c.config.gateway = a.ipv4_gateways.size() > 0 ? a.ipv4_gateways.front() : SysIP4();
        for (i32 i = 0; i < a.ipv4_dns.size() && i < SYS_NET_CONFIG_MAX_DNS; i++)
            c.config.dns[i] = a.ipv4_dns.size() > 0 ? a.ipv4_dns[i] : SysIP4();
        c.config.dhcp_enabled = a.dhcpv4_enabled;
        c.config.ddns_enabled = a.ddns_enabled;
        s_modified_adapters.push_back(c);
        s_current_adapter_configs.push_back(c.config);
    }
}

void NetworkingInit(NetworkData** nd)
{
    ZoneScopedN("Networking Init");
    VALIDATE(nd && !(*nd));
    *nd = new NetworkData();
    VALIDATE(*nd);
#if _DEBUG
    double start = SysGetTime();
#endif

    //TODO: Async these:
    ReadEnvironmentVariables(&s_network.env, s_network.env_filename);
    (*nd)->is_admin = SysHasAdminPrivledge();
    UpdateNetworkAdaptersInfo(*nd);

#if _DEBUG
    double end = SysGetTime();
    float total_time = float((end - start) * 1000);
    DebugPrint("Time to get response: %fms", total_time);
    i32 test = 1;
#endif
}
void NetworkingDestroy(NetworkData** network_data)
{
    VALIDATE(network_data && *network_data);
    delete (*network_data);
}

bool NetAdapterConfigsMatchIPs(const SysNetAdapterConfig& a, const SysNetAdapterConfig& b)
{
    ZoneScoped;
    bool r = true;
    r &= a.ip.ip.addr == b.ip.ip.addr;
    r &= a.ip.subnet.mask == b.ip.subnet.mask;
    r &= a.gateway.addr == b.gateway.addr;
    r &= a.dhcp_enabled == b.dhcp_enabled;
    return r;
}
bool NetAdapterConfigsMatchDNS(const SysNetAdapterConfig& a, const SysNetAdapterConfig& b)
{
    bool r = true;
    r &= a.ddns_enabled == b.ddns_enabled;
    for (i32 i = 0; i < SYS_NET_CONFIG_MAX_DNS; i++)
        r &= a.dns[i].addr == b.dns[i].addr;
    return r;
}

const char* GetConfigsForImgui(void* user_data, int idx)
{
    VALIDATE_V(idx < g_network_settings.configs.size(), nullptr);
    VALIDATE_V(idx >= 0, nullptr);
    return g_network_settings.configs[idx].name.c_str();
}

void NetworkImgui(NetworkData& data)
{
    ZoneScoped;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    Threading& threading = Threading::GetInstance();
    ImGuiWindowFlags section_flags =
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoMove;

    static SysNetAdapterConfig adapter_config_save = {};
    static bool create_popup_open = false;
    static i32 set_adapter_index = 0;
    static bool set_popup_open = false;

    #define ADAPTERS_TITLE "Adapters"
    if (ImGui::BeginChild(ADAPTERS_TITLE, { 0, 0 }, true, section_flags))
    {
        ZoneScopedN(ADAPTERS_TITLE);
        ImguiTextCentered(ADAPTERS_TITLE);
        ImGui::NewLine();
        if (!data.is_admin)
            ImguiTextCentered("WARNING!:  Must be ran as admin for this feature!", &Yellow);

        std::error_code ec;
        ImGui::BeginDisabled(!data.is_admin);
        float height = 40;
        const ImVec2 adapter_child_size(300.0f, 300.0f);
        const float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
        for (i32 i = 0; i < s_modified_adapters.size(); i++)
        {
            MainAdapterInfo& a = s_modified_adapters[i];
            ImGui::PushID(i);
            //ImGui::BeginDisabled(FlagIntersects(s.completed, AsyncStatus_Completed));
            if (ImGui::BeginChild("##AdapterChild", adapter_child_size, true, section_flags))
            {
                SysNetAdapterConfig& c = a.config;
                ImguiTextCentered(c.name);
                ImGui::Separator();
                ImGui::PushFont(g_data.fonts[FontIndex_Small]);
                ImguiTextCentered(a.desc);
                ImGui::PopFont();
                ImguiEdit(c);
            }

            //ImGui::SameLine();
            if (ImGui::Button("Set From Config", ImVec2(-1, 0)))
            {
                adapter_config_save = s_modified_adapters[i].config;
                adapter_config_save.name.clear();
                set_popup_open = true;
                set_adapter_index = i;
            }


            const bool ips_match = NetAdapterConfigsMatchIPs(a.config, s_current_adapter_configs[i]);
            const bool dns_match = NetAdapterConfigsMatchDNS(a.config, s_current_adapter_configs[i]);
            ImGui::BeginDisabled(ips_match && dns_match);
            const float button_count = 2;
            const ImVec2 button_size = { (adapter_child_size.x / button_count) -
                                         (ImGui::GetStyle().FramePadding.x) -
                                         (ImGui::GetStyle().ItemSpacing.x * (button_count - 1)), 30};
            if (ImGui::Button("Apply", button_size))
            {
                if (!ips_match)
                {
                    SysSetNetAdapterIP(a.guid, a.config, s_current_adapter_configs[i]);
                    UpdateNetworkAdaptersInfo(&data);
                }
                if (!dns_match)
                {
                    SysSetNetAdapterDNS(a.guid, a.config, s_current_adapter_configs[i]);
                    UpdateNetworkAdaptersInfo(&data);
                }
            }
            ImGui::EndDisabled();
			ImGui::SameLine();
            if (ImGui::Button("Create Config", button_size))
            {
                adapter_config_save = s_modified_adapters[i].config;
                adapter_config_save.name.clear();
                create_popup_open = true;
            }
            ImGui::EndChild();
            ImGui::PopID();

            float last_button_x2 = ImGui::GetItemRectMax().x;
            float next_button_x2 = last_button_x2 + ImGui::GetStyle().ItemSpacing.x + adapter_child_size.x; // Expected position if next button was on same line

            float text_start = ImGui::GetCursorPosX() + ImGui::GetStyle().ItemSpacing.x / 2;
            if (i + 1 < data.adapters.data.size() && next_button_x2 < window_visible_x2)
                ImGui::SameLine();
        }
        ImGui::EndDisabled();
    }
    ImGui::EndChild();

    const ImVec2 popup_size = { 300, 350 };//{ 1024, 600 }
    if (create_popup_open)
    {
        ImGui::SetNextWindowSize(popup_size);
        ImGui::SetNextWindowPos((viewport->Size - popup_size) / 2.0f);
        const char* config_popup_name = "IP Configuration Popup";
        ImGui::OpenPopup(config_popup_name);
        if (ImGui::BeginPopupModal(config_popup_name, NULL, ImGuiWindowFlags_NoNav | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
        {
            ImGui::SeparatorText("Config To Be Saved");
            ImguiEdit(adapter_config_save.name, "Configuration Name", "Name:");
            ImguiEdit(adapter_config_save);

            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            const float button_count = 2;
            const ImVec2 button_size = { (popup_size.x / button_count) -
                                         (ImGui::GetStyle().FramePadding.x) -
                                         (ImGui::GetStyle().ItemSpacing.x * (button_count - 1)), 30};
            ImGui::BeginDisabled(adapter_config_save.name.size() < 2);
            if (ImGui::Button("Save", button_size))
            {
                g_network_settings.configs.push_back(adapter_config_save);
                WriteSettings();
                ImGui::CloseCurrentPopup();
                create_popup_open = false;
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            if (ImGui::Button("Cancel", button_size))
            {
                ImGui::CloseCurrentPopup();
                create_popup_open = false;
            }

            ImGui::EndPopup();
        }
    }

    if (set_popup_open)
    {
        ImGui::SetNextWindowSize(popup_size);
        ImGui::SetNextWindowPos((viewport->Size - popup_size) / 2.0f);
        const char* config_popup_name = "IP Set Popup";
        ImGui::OpenPopup(config_popup_name);
        if (ImGui::BeginPopupModal(config_popup_name, NULL, ImGuiWindowFlags_NoNav | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
        {

            if (set_adapter_index < 0 || set_adapter_index >= s_modified_adapters.size())
            {
                DebugPrint("Invalid adapter index: %i", set_adapter_index);
                return;
            }
            const MainAdapterInfo& adapter = s_modified_adapters[set_adapter_index];

            ImGui::Text("Config:");
            ImGui::SameLine();
            ImGui::PushItemWidth(175);
            static i32 config_selection = 0;
            if (ImGui::Combo("##Config", &config_selection, GetConfigsForImgui, nullptr, (i32)g_network_settings.configs.size(), -1))
            {
            }
            if (config_selection < 0 || config_selection >= s_modified_adapters.size())
            {
                DebugPrint("Invalid config index: %i", config_selection);
                return;
            }
            const SysNetAdapterConfig& config = g_network_settings.configs[config_selection];

            ImGui::SeparatorText("Config To Set");
            ImguiView(config);

            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            const float button_count = 2;
            const ImVec2 button_size = { (popup_size.x / button_count) -
                                         (ImGui::GetStyle().FramePadding.x) -
                                         (ImGui::GetStyle().ItemSpacing.x * (button_count - 1)), 30};
            ImGui::BeginDisabled(adapter_config_save.name.size() < 2);
            if (ImGui::Button("Set", button_size))
            {
                SysSetNetAdapterIP(adapter.guid, config, adapter.config);
                UpdateNetworkAdaptersInfo(&data);
                set_popup_open = false;
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            if (ImGui::Button("Cancel", button_size))
            {
                ImGui::CloseCurrentPopup();
                set_popup_open = false;
            }

            ImGui::EndPopup();
        }
    }
}

void NetworkShutdown()
{

}
