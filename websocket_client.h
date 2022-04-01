#pragma once
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <memory>
#include <string>

namespace websocket_client {

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>


class Session : public std::enable_shared_from_this<Session>
{
    std::string request_;
    tcp::resolver resolver_;
    websocket::stream<beast::ssl_stream<beast::tcp_stream>> ws_;
    long delay_time_;

public:
    explicit Session(net::io_context& ioc, ssl::context& ctx);

    void run(long delay_time, char const* symbol_name);

private:
    class DelayedOutput;
    typedef std::shared_ptr<DelayedOutput> DelayedOutputPtr;

    void on_resolve(beast::error_code ec, tcp::resolver::results_type results);
    void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep);
    void on_ssl_handshake(beast::error_code ec);
    void on_handshake(beast::error_code ec);
    void on_write(beast::error_code ec, std::size_t bytes_transferred);
    void on_read(DelayedOutputPtr op, beast::error_code ec, std::size_t bytes_transferred);

    void async_read_into_new_delayed_operation();
};

} // namespace websocket_client
