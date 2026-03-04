#include "asio.hpp"
#include "asio/ts/buffer.hpp"
#include "asio/ts/internet.hpp"

#include "Networking.h"
#include "WinInterop.h"

#include <vector>

struct NetworkInfo {
    asio::io_context context;
};
NetworkInfo g_network;

void NetworkingInit()
{
    asio::error_code ec;
    asio::ip::tcp::endpoint endpoint(asio::ip::make_address("93.184.216.34", ec), 80);
    asio::ip::tcp::socket socket(g_network.context);
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
        std::string request =
            "Get /index.html HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "Connection: close\r\n\r\n";
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
