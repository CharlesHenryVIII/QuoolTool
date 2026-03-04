#if 1
#include "curl/curl.h"

#include "Networking.h"
#include "WinInterop.h"


struct NetworkInfo {
    const std::string url = "https://api.github.com/repos/CharlesHenryVIII/UATHelper/releases/latest";
    CURL* curl;
};
NetworkInfo s_network;

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* out)
{
    out->append((char*)contents, size * nmemb);
    return size * nmemb;
}

void NetworkingInit()
{
    double start = SysGetTime();
    s_network.curl = curl_easy_init();
    std::string response;

    curl_easy_setopt(s_network.curl, CURLOPT_URL, s_network.url.c_str());

    curl_easy_setopt(s_network.curl, CURLOPT_USERAGENT, "MyAppUpdater");
    curl_easy_setopt(s_network.curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(s_network.curl, CURLOPT_WRITEDATA, &response);

    curl_easy_perform(s_network.curl);
    curl_easy_cleanup(s_network.curl);
    double end = SysGetTime();
    float total_time = float((end - start) * 1000);
    DebugPrint("Time to get response: %fms", total_time);
    i32 i = 0;
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
