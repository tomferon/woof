module;

#include <iostream>
#include <memory>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

module woof.http_server;

using namespace woof::http_server;

namespace ip = boost::asio::ip;
using tcp = ip::tcp;
namespace http = boost::beast::http;
using namespace std::literals;

static const auto router =
    "test" / capture(std::function{[](std::string_view str) {
        return method(
            http::verb::get,
            std::function{[str](const http::request<http::string_body>& request) {
                auto response = http::response<http::string_body>{http::status::ok, request.version()};
                response.body() = str;
                return response;
            }});
    }});

void Connection::readRequest()
{
    const auto conn = shared_from_this();

    request = {};
    boost::asio::steady_timer timer(socket.get_executor());
    timer.expires_after(std::chrono::seconds(10));
    timer.async_wait([=](boost::system::error_code ec) {
        conn->close();
        std::cout << "Timed out with " << ec.message() << std::endl;
    });

    std::cout << "Reading request..." << std::endl;

    async_read(socket, buffer, request, [=](boost::system::error_code ec, std::size_t bytes) {
        std::cout << "Bytes read: " << bytes << ", error message: " << ec.message() << std::endl;
        // FIXME: handle ec
        if (!ec) {
            conn->handleRequest();
        } else {
            std::cout << ec.message() << std::endl;
            conn->close();
        }
    });
}

std::vector<std::string_view> splitFragments(std::string_view path)
{
    std::vector<std::string_view> fragments{};
    std::string_view::size_type left{0}, right{0};
    while ((right = path.find_first_of('/', left)) != std::string_view::npos) {
        if (left != 0 || right != 0)
            fragments.push_back(path.substr(left, right - left));
        left = right + 1; // Skip / for next iteration.
    }
    right = path.size();
    if (left != right) {
        fragments.push_back(path.substr(left, right - left));
    }
    return fragments;
}

void Connection::handleRequest()
{
    auto response = std::make_shared<http::response<http::string_body>>(
        http::status::internal_server_error, request.version());
    try {
        std::vector<std::string_view> fragments = splitFragments(request.target());
        *response = router->handle(request.method(), std::span{fragments.data(), fragments.size()}, request);
    } catch (const RouteDoesntMatch&) {
        *response = http::response<http::string_body>(http::status::not_found, request.version());
    }

    // FIXME: Capture version from CMake and return Server: Woof/X.Y
    response->set(http::field::server, BOOST_BEAST_VERSION_STRING);
    bool keepAlive = request.keep_alive();
    response->keep_alive(keepAlive);
    response->prepare_payload();

    auto conn = shared_from_this();
    // The response pointer is passed to the continuation to ensure it's not deleted before async_write had a chance to
    // send it to the client.
    // FIXME: Tell linter to not flag 'response' in the following line:
    http::async_write(socket, *response, [conn, response, keepAlive](boost::system::error_code ec, std::size_t bytes) {
        std::cout << "Bytes written: " << bytes << ", error message: " << ec.message() << std::endl;
        // FIXME: Handle ec
        if (keepAlive) {
            conn->readRequest();
        } else {
            conn->close();
        }
    });
}

void Connection::close()
{
    boost::system::error_code ec;
    auto ec2 = socket.shutdown(tcp::socket::shutdown_send, ec);
    std::cout << ec.message() << std::endl;
    std::cout << ec2.message() << std::endl;
}

Server::Server(const std::string_view host, const unsigned short port)
{
    const auto endpoint = tcp::endpoint{ip::make_address(host), port};
    // FIXME: handle errors
    acceptor.open(endpoint.protocol());
    acceptor.set_option(tcp::acceptor::reuse_address(true)); // FIXME: Why?
    acceptor.bind(endpoint);
    acceptor.listen();
    // Indirection since, otherwise, the weak pointer has not been initialized yet.
    std::shared_ptr<Server>{this}->accept();
}

void Server::accept()
{
    const auto conn = Connection::construct(ioContext);
    const auto server = shared_from_this();
    acceptor.async_accept(conn->getSocket(), [=](boost::system::error_code ec) {
        // FIXME: handle ec
        conn->readRequest();
        server->accept();
    });
}

void Server::handleRequests()
{
    ioContext.run();
}
