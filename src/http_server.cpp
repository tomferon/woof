module;

#include <iostream>
#include <memory>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <nlohmann/json.hpp>

module woof.http_server;

import woof.api;
import woof.jobs;
import woof.engine;

using namespace woof::http_server;

namespace ip = boost::asio::ip;
using tcp = ip::tcp;
namespace http = boost::beast::http;
using namespace std::literals;

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

// FIXME: Reimplement using string_view to be more idiomatic.
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

std::unordered_map<std::string, std::string> parseQueryString(const http::request<http::string_body>& request)
{
    std::unordered_map<std::string, std::string> params{};
    std::string_view path{request.target()};
    if (const auto startPos = path.find('?'); startPos != std::string_view::npos) {
        path.remove_prefix(startPos);
        while (!path.empty()) {
            path.remove_prefix(1); // Skip ? or &
            const auto ampPos = path.find('&');
            std::string_view kvString{path.substr(0, ampPos)};
            const auto eqPos = kvString.find('=');
            const std::string key{kvString.substr(0, eqPos)};
            const std::string value{kvString.substr(eqPos + 1)};
            params[key] = value;
            if (ampPos == std::string_view::npos) {
                break;
            }
            path.remove_prefix(ampPos);
        }
    }
    return params;
}

void Connection::handleRequest()
{
    auto response = std::make_shared<http::response<http::string_body>>(handler(request));
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

Server::Server(const std::string_view host, const unsigned short port,
    std::function<http::response<http::string_body>(const http::request<http::string_body>&)> handler)
    : handler{std::move(handler)}
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
    const auto conn = Connection::construct(ioContext, handler);
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

http::response<http::string_body> Handler::handleRequest(const http::request<http::string_body>& request)
{
    try {
        std::vector<std::string_view> fragments = splitFragments(request.target());
        return router->handle(request.method(), std::span{fragments.data(), fragments.size()}, request);
    } catch (const RouteDoesntMatch&) {
        return http::response<http::string_body>{http::status::not_found, request.version()};
    }
}

auto Handler::makeRouter()
    -> std::unique_ptr<Matcher<http::response<http::string_body>, const http::request<http::string_body>&>>
{
    using namespace std::placeholders;
    using handler = http::response<http::string_body>(const http::request<http::string_body>&);

    auto getHealthEndpoint = "health" / method(
        http::verb::get,
        std::function<handler>{[this](const auto& request) { return getHealth(request); }}
    );

    auto jobsEndpoints = "jobs" / routes({
        capture(std::function{
            [this](JobId jobId) {
                return method(http::verb::get,
                    std::function<handler>{[this, jobId](const auto& request) { return getJob(jobId, request); }});
            }
        }),
        method(http::verb::post, std::function<handler>{[this](const auto& request) { return postJob(request); }}),
    });

    return routes({std::move(getHealthEndpoint), std::move(jobsEndpoints)});
}

http::response<http::string_body> Handler::getHealth(const http::request<http::string_body>& request)
{
    auto response = http::response<http::string_body>{http::status::ok, request.version()};
    response.set(http::field::content_type, "text/plain");
    response.body() = "Healthy";
    return response;
}

http::response<http::string_body> Handler::getJob(const JobId jobId, const http::request<http::string_body>& request)
{
    try {
        const auto params = parseQueryString(request);
        const bool closure = params.contains("closure") && params.at("closure") == "true";
        const auto jobs = server.getJobs(jobId, closure);
        auto response = http::response<http::string_body>{http::status::ok, request.version()};
        response.set(http::field::content_type, "application/json");
        response.body() = api::JobSetFrom{jobs}.toJson().dump();
        return response;
    } catch (const JobNotFound&) {
        return http::response<http::string_body>{http::status::not_found, request.version()};
    }
}

http::response<http::string_body> Handler::postJob(const http::request<http::string_body>& request)
{
    // FIXME: Handle wrong body
    const auto jobSpecs = api::JobSetTo{nlohmann::json::parse(request.body())}.toJobSpecs();
    const auto jobIds = server.addJobs(jobSpecs);

    auto response = http::response<http::string_body>{http::status::created, request.version()};
    response.set(http::field::content_type, "application/json");
    auto jobIdsJson = jobIds
        | std::views::transform([](const auto& pair) {
            auto [name, jobId] = pair;
            return std::pair{name, jobId.toString()};
        })
        | std::ranges::to<std::map<std::string, std::string>>();
    response.body() = nlohmann::json{{"jobIds", jobIdsJson}}.dump();
    return response;
}
