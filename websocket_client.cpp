#include "websocket_client.h"
#include <boost/asio/strand.hpp>
#include <boost/bind/bind.hpp>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>

using namespace websocket_client;

static const std::string HOST = "ws.binaryws.com";
static const std::string PORT = "443";
static const std::string TARGET = "/websockets/v3?app_id=1089";


static void fail(beast::error_code ec, char const* what) {
    std::cerr << what << ": " << ec.message() << std::endl;
}

///////////////////////////////////////
// class Session::DelayedOutput
//
class Session::DelayedOutput : public std::enable_shared_from_this<Session::DelayedOutput>
{
    beast::flat_buffer buffer_;
    net::deadline_timer timer_;
    long delay_time_;
    static const boost::regex rgx;

public:
    explicit DelayedOutput(const net::deadline_timer::executor_type& ex)
        : timer_(ex)
    { }

    DelayedOutput(const DelayedOutput&) = default;
    DelayedOutput(DelayedOutput&&) = default;

    beast::flat_buffer& get_buffer() {
        return buffer_;
    }

    void delay(long delay_time) {
        delay_time_ = delay_time;
        timer_.expires_from_now(boost::posix_time::seconds(delay_time_));
        timer_.async_wait(
            beast::bind_front_handler(&DelayedOutput::on_timer, shared_from_this()));
    }

private:
    void on_timer(const boost::system::error_code& ec) {
        if(ec) {
            return fail(ec, "timer");
        }
        
        auto msg = beast::buffers_to_string(buffer_.data());
        auto result = boost::regex_replace(msg, rgx, [this](const boost::smatch& m) {
            auto tm = boost::lexical_cast<std::time_t>(m[2]);
            return m[1] + boost::lexical_cast<std::string>(tm + this->delay_time_);
        });

        std::cout << result << std::endl;
    }
};

const boost::regex Session::DelayedOutput::rgx("(\"epoch\":)(\\d+)");

///////////////////////////////////////
// class Session
//
Session::Session(net::io_context& ioc, ssl::context& ctx)
    : resolver_(net::make_strand(ioc)), ws_(net::make_strand(ioc), ctx)
{ }

void Session::run(long delay_time, char const* symbol_name) {
    delay_time_ = delay_time;
    request_ =  std::string("{\"ticks\": \"") + symbol_name + "\"}";

    // Look up the domain name
    resolver_.async_resolve(HOST, PORT,
        beast::bind_front_handler(&Session::on_resolve, shared_from_this()));
}

void Session::on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
    if(ec) {
        return fail(ec, "resolve");
    }

    // Set a timeout on the operation
    beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

    // Make the connection on the IP address we get from a lookup
    beast::get_lowest_layer(ws_).async_connect(results,
        beast::bind_front_handler(&Session::on_connect, shared_from_this()));
}

void Session::on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep) {
    if(ec) {
        return fail(ec, "connect");
    }

    // Set a timeout on the operation
    beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

    // Set SNI Hostname (many hosts need this to handshake successfully)
    if( !SSL_set_tlsext_host_name(ws_.next_layer().native_handle(), HOST.c_str()) ) {
        ec = beast::error_code(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category());
        return fail(ec, "connect");
    }
  
    // Perform the SSL handshake
    ws_.next_layer().async_handshake(ssl::stream_base::client,
        beast::bind_front_handler(&Session::on_ssl_handshake, shared_from_this()));
}

void Session::on_ssl_handshake(beast::error_code ec) {
    if(ec) {
        return fail(ec, "ssl_handshake");
    }

    // Turn off the timeout on the tcp_stream, because
    // the websocket stream has its own timeout system.
    beast::get_lowest_layer(ws_).expires_never();

    // Set timeout settings for the websocket
    auto timeouts = websocket::stream_base::timeout::suggested(beast::role_type::client);
    //timeouts.idle_timeout = std::chrono::seconds(300);      // suggested timeout is none()
    ws_.set_option(std::move(timeouts));

    // Set a decorator to change the User-Agent of the handshake
    ws_.set_option(websocket::stream_base::decorator(
        [](websocket::request_type& req) {
            req.set(beast::http::field::user_agent, "test-websocket-client");
        }));

    // Update the host_ string. This will provide the value of the
    // Host HTTP header during the WebSocket handshake.
    // See https://tools.ietf.org/html/rfc7230#section-5.4
    auto host = HOST + ':' + PORT;

    // Perform the websocket handshake
    ws_.async_handshake(host, TARGET,
        beast::bind_front_handler(&Session::on_handshake, shared_from_this()));
}

void Session::on_handshake(beast::error_code ec) {
    if(ec) {
        return fail(ec, "handshake");
    }

    // Send the request
    ws_.async_write(net::buffer(request_),
        beast::bind_front_handler(&Session::on_write, shared_from_this()));
}

void Session::on_write(beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);
    if(ec) {
        return fail(ec, "write");
    }

    std::cerr << "Connection established. Witing for information..." << std::endl;
    async_read_into_new_delayed_operation();
}

void Session::on_read(DelayedOutputPtr op, beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);
    if(ec) {
        return fail(ec, "read");
    }

    std::cerr << "Information received and delayed..." << std::endl;
    op->delay(delay_time_);

    async_read_into_new_delayed_operation();
}

void Session::async_read_into_new_delayed_operation() {
    auto op = std::make_shared<DelayedOutput>(net::make_strand(ws_.get_executor()));
    ws_.async_read(op->get_buffer(),
        beast::bind_front_handler(&Session::on_read, shared_from_this(), op));
}
