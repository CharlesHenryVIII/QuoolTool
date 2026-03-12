#include "curl/curl.h"

#include "Networking.h"
#include "WinInterop.h"
#include "LoadJson.h"

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
        std::string auth = "Authorization: Bearer " + s_network.env.github_api_key;
        headers = curl_slist_append(headers, "Accept: application/octet-stream");
        headers = curl_slist_append(headers, auth.c_str());
        headers = curl_slist_append(headers, "X-GitHub-Api-Version: 2022-11-28");
        CURLCHECK(curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers));
    }

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
    std::string filename = ToString("QuoolTool_v%i_%i.zip", g_online_version.major, g_online_version.minor);
    if (response.data.size() > Megabytes(1))
    {
        std::fstream file(filename, std::ios_base::out | std::ios_base::binary);
        if (!file.good())
        {
            DebugPrint("Failed to open file for write: %s", filename.c_str());
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
    UnzipArchive(filename, "", filenames);
    if (filenames.size())
    {
        if (filenames[0].find("QuoolTool") != std::string::npos)
        {
            Path fe = filename;
            std::string filename_no_ext = fe.stem().string() + ".exe";
            std::error_code ec;
            fs::rename(filenames[0], filename_no_ext, ec);
            if (ec)
            {
                DebugPrint("Error: failed to rename file: \"%s\" to \"%s\"", filenames[0].c_str(), filename_no_ext.c_str());
                DebugPrint("\"create_directories\" failure: \"%d\", \"%s\"", ec.value(), ec.message().c_str());
                FAIL;
                g_download_state = AsyncStatus_FetchedFailed;
                return;
            }
            fs::remove(filename, ec);
            if (ec)
            {
                DebugPrint("Error: failed to remove file: \"%s\"", filename.c_str());
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
    auto json = Json::parse(response);
    if (!JsonSafeGet(tag, &json, "tag_name"))
    {
        DebugPrint("Error: failed to get tag_name, url: %s", s_network.url.c_str());
        DebugPrint("    json response vvvvvv");
        DebugPrint("%s", response.c_str());
        g_download_state = AsyncStatus_FetchedFailed;
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
    g_download_state = AsyncStatus_FetchedSuccess;
}

void NetworkingInit()
{
    ZoneScopedN("Networking Init");
#if _DEBUG
    double start = SysGetTime();
#endif

    ReadEnvironmentVariables(&s_network.env, s_network.env_filename);

#if _DEBUG
    double end = SysGetTime();
    float total_time = float((end - start) * 1000);
    DebugPrint("Time to get response: %fms", total_time);
    i32 test = 1;
#endif
}

void NetworkShutdown()
{

}
