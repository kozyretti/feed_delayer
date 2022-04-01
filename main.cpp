#include <iostream>
#include "websocket_client.h"
#include "root_certificates.hpp"

namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>

int main(int argc, char** argv) {
    if(argc < 3)
    {
        std::cerr << "Usage: feed_delayer <delay_in_seconds> <symbol_name>" << std::endl
                  << "Example:" << std::endl
                  << "    feed_delayer 60 R_100" << std::endl;
        return EXIT_FAILURE;
    }

    long delay;
    try {
        delay = std::stol(argv[1]);
        if(delay < 0) {
            throw(std::exception());
        }
        std::cerr << "Delay time " << delay << " sec." << std::endl;
    }
    catch(const std::exception&) {
        std::cerr << "Wrong delay time: " << argv[1] << std::endl;
        return EXIT_FAILURE;
    }

    const char* symbol_name = argv[2];

    net::io_context ioc;
    ssl::context ctx{ssl::context::tlsv12_client};
    load_root_certificates(ctx);

    std::make_shared<websocket_client::Session>(ioc, ctx)->run(delay, symbol_name);
    ioc.run();

    return EXIT_SUCCESS;
}
