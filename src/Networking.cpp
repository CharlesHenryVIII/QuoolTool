#if 1
#include "curl/curl.h"

#include "Networking.h"
#include "WinInterop.h"
#include "LoadJson.h"
#include "Version.h"

#include "json.hpp"

#include <fstream>
#include <iostream>

using Json = nlohmann::json;

#ifdef WIN32
#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

struct NetworkInfo {
    const std::string url = "https://api.github.com/repos/CharlesHenryVIII/ScadaBackup/releases/latest";
    const std::wstring env_filename = L".env";
    EnvironmentVariables env;
    Version online_version = {};
};
NetworkInfo s_network;

std::string GetUrlFromVersion(Version v)
{
    std::string r = ToString("https://github.com/CharlesHenryVIII/ScadaBackup/releases/download/%s/ScadaBackup_windows_x64_Release.exe", v.AsString().c_str());
    return r;
}

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* out)
{
    out->append((char*)contents, size * nmemb);
    return size * nmemb;
}

#define CURLCHECK(fun)  \
{\
    CURLcode result = fun;\
    if (result != CURLE_OK)\
    {\
        DebugPrint("Error: \"%s\" failed at %s(%i) CURLcode: %i ", #fun, __FILENAME__, __LINE__, result);\
    }\
} REQUIRE_SEMICOLON

void GetGithubReleaseInfo()
{
    ZoneScopedN("Networking GetGithubReleaseInfo");

    CURL* curl = curl_easy_init();
    struct curl_slist* headers = nullptr;
    if (s_network.env.github_api_key.size() > 10)
    {
        std::string auth = "Authorization: Bearer" + s_network.env.github_api_key;
        headers = curl_slist_append(headers, auth.c_str());
        headers = curl_slist_append(headers, "X-GitHub-Api-Version: 2022-11-28");
        headers = curl_slist_append(headers, "Accept: application/vnd.github+json");
        CURLCHECK(curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers));
    }

    std::string response;
    CURLCHECK(curl_easy_setopt(curl, CURLOPT_URL, s_network.url.c_str()));
    CURLCHECK(curl_easy_setopt(curl, CURLOPT_USERAGENT, "QuoolToolUpdater"));
    CURLCHECK(curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback));
    CURLCHECK(curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response));

    CURLCHECK(curl_easy_perform(curl));

    if (s_network.env.github_api_key.size() > 10)
    {
        curl_slist_free_all(headers);
    }
    curl_easy_cleanup(curl);

    auto json = Json::parse(response);
    std::string tag = json["tag_name"];
    s_network.online_version.SetFromTag(tag);
}

void UpdateScadaBackup()
{
    ZoneScopedN("Networking GetGithubReleaseInfo");

    CURL* curl = curl_easy_init();
    struct curl_slist* headers = nullptr;
    if (s_network.env.github_api_key.size() > 10)
    {
        std::string auth = "Authorization: Bearer" + s_network.env.github_api_key;
        headers = curl_slist_append(headers, auth.c_str());
        headers = curl_slist_append(headers, "X-GitHub-Api-Version: 2022-11-28");
        headers = curl_slist_append(headers, "Accept: application/vnd.github+json");
        CURLCHECK(curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers));
    }

    std::vector<u8> response;
    CURLCHECK(curl_easy_setopt(curl, CURLOPT_URL, GetUrlFromVersion(s_network.online_version)));
    CURLCHECK(curl_easy_setopt(curl, CURLOPT_USERAGENT, "QuoolToolUpdater"));
    CURLCHECK(curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback));
    CURLCHECK(curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response));

    CURLCHECK(curl_easy_perform(curl));

    if (s_network.env.github_api_key.size() > 10)
    {
        curl_slist_free_all(headers);
    }
    curl_easy_cleanup(curl);
}

void NetworkingInit()
{
    ZoneScopedN("Networking Init");
#if _DEBUG
    double start = SysGetTime();
#endif

    ReadEnvironmentVariables(&s_network.env, s_network.env_filename);

    GetGithubReleaseInfo();
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


#else
#include "asio.hpp"
#include "asio/ts/buffer.hpp"
#include "asio/ts/internet.hpp"

#include "Networking.h"
#include "WinInterop.h"

#include <vector>


struct NetworkInfo {
    asio::io_context context;
};
NetworkInfo s_network;

void NetworkingInit()
{
    asio::error_code ec;
    asio::ip::tcp::endpoint endpoint(asio::ip::make_address("93.184.216.34", ec), 80);
    asio::ip::tcp::socket socket(s_network.context);
    socket.connect(endpoint, ec);
    if (!ec)
    {
        DebugPrint("Connected!");
    }
    else
    {
        DebugPrint("Connected!");
    }

    if (socket.is_open())
    {
        std::string request = "GET https://api.github.com/repos/CharlesHenryVIII/UATHelper/releases/latest\r\n\r\n";
            //"Get /index.html HTTP/1.1\r\n"
            //"Host: example.com\r\n"
            //"Connection: close\r\n\r\n";
        socket.write_some(asio::buffer(request.data(), request.size()), ec);

        socket.wait(socket.wait_read);

        size_t bytes = socket.available();
        DebugPrint("Bytes Available: %i", bytes);

        if (bytes > 0)
        {
            std::vector<char> buffer(bytes);
            socket.read_some(asio::buffer(buffer.data(), buffer.size()), ec);
        }
    }

    i32 test = 1;
}
#endif
