module;

#include <span>
#include <boost/beast/core.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>

export module woof.http_server;

import woof.jobs;
import woof.server;

namespace ip = boost::asio::ip;
using tcp = ip::tcp;
namespace http = boost::beast::http;

namespace woof::http_server {
export struct RouteDoesntMatch final : std::exception { };

export template<typename Result, typename... Params>
class Matcher {
public:
    virtual Result handle(http::verb, std::span<std::string_view>, Params...) const = 0;

    virtual ~Matcher() = default;
};

export template<typename Result, typename... Params>
class MethodMatcher final : public Matcher<Result, Params...> {
public:
    explicit MethodMatcher(const http::verb verb, std::function<Result(Params...)>&& handler)
        : verbToMatch{verb}, handler{std::move(handler)} { }

    Result handle(const http::verb verb, const std::span<std::string_view> fragments, Params... params) const override
    {
        if (verb != verbToMatch || !fragments.empty())
            throw RouteDoesntMatch{};
        return handler(params...);
    }

private:
    http::verb verbToMatch;
    std::function<Result(Params...)> handler;
};

export template<typename Result, typename... Params>
std::unique_ptr<Matcher<Result, Params...>> method(const http::verb verb, std::function<Result(Params...)>&& handler)
{
    return std::make_unique<MethodMatcher<Result, Params...>>(verb, std::move(handler));
}

export template<typename Result, typename... Params>
class StringMatcher final : public Matcher<Result, Params...> {
public:
    explicit StringMatcher(std::string&& str, std::unique_ptr<Matcher<Result, Params...>>&& matcher)
        : stringToMatch{std::move(str)}, matcher{std::move(matcher)}
    { }

    Result handle(const http::verb verb, const std::span<std::string_view> strs, Params... params) const override
    {
        if (!strs.empty() && strs.front() == stringToMatch)
            return matcher->handle(verb, strs.subspan(1), params...);
        throw RouteDoesntMatch{};
    }

private:
    std::string stringToMatch;
    std::unique_ptr<Matcher<Result, Params...>> matcher;
};

export template<typename Result, typename... Params>
auto operator/(std::string&& str, std::unique_ptr<Matcher<Result, Params...>>&& matcher)
    -> std::unique_ptr<Matcher<Result, Params...>>
{
    return std::make_unique<StringMatcher<Result, Params...>>(std::move(str), std::move(matcher));
}

export template<typename Result, typename... Params>
class CaptureMatcher final : public Matcher<Result, Params...> {
public:
    explicit CaptureMatcher(std::function<std::unique_ptr<Matcher<Result, Params...>>(std::string_view)>&& makeMatcher)
        : makeMatcher{std::move(makeMatcher)}
    { }

    template<typename T>
        requires (!std::is_same_v<T, std::string_view>)
    explicit CaptureMatcher(std::function<std::unique_ptr<Matcher<Result, Params...>>(T)>&& makeMatcher)
        : makeMatcher{
            [makeMatcher](const std::string_view str) {
                T x;
                std::stringstream ss{std::string{str}};
                ss >> x;
                if (ss.fail())
                    throw RouteDoesntMatch{};
                return makeMatcher(x);
            }
        }
    { }

    Result handle(const http::verb verb, const std::span<std::string_view> strs, Params... params) const override
    {
        if (strs.empty())
            throw RouteDoesntMatch{};
        return makeMatcher(strs.front())->handle(verb, strs.subspan(1), params...);
    }

private:
    std::function<std::unique_ptr<Matcher<Result, Params...>>(std::string_view)> makeMatcher;
};

export template<typename T, typename Result, typename... Params>
auto capture(std::function<std::unique_ptr<Matcher<Result, Params...>>(T)>&& makeMatcher)
    -> std::unique_ptr<Matcher<Result, Params...>>
{
    return std::make_unique<CaptureMatcher<Result, Params...>>(std::move(makeMatcher));
}

export template<typename Result, typename... Params>
class RoutesMatcher final : public Matcher<Result, Params...> {
public:
    RoutesMatcher(std::initializer_list<std::unique_ptr<Matcher<Result, Params...>>>&& ms)
    {
        matchers.reserve(ms.size());
        for (auto& matcher: ms) {
            matchers.push_back(std::move(const_cast<std::unique_ptr<Matcher<Result, Params...>>&>(matcher)));
        }
    }

    Result handle(const http::verb verb, const std::span<std::string_view> strs, Params... params) const override
    {
        for (const auto& matcher: matchers) {
            try {
                return matcher->handle(verb, strs, params...);
            } catch (const RouteDoesntMatch&) { }
        }
        throw RouteDoesntMatch{};
    }

private:
    std::vector<std::unique_ptr<Matcher<Result, Params...>>> matchers;
};

export template<typename Result, typename... Params>
auto routes(std::initializer_list<std::unique_ptr<Matcher<Result, Params...>>>&& matchers)
    -> std::unique_ptr<Matcher<Result, Params...>>
{
    return std::make_unique<RoutesMatcher<Result, Params...>>(std::move(matchers));
}

using RequestHandler = std::function<http::response<http::string_body>(const http::request<http::string_body>&)>;

class Connection final : public std::enable_shared_from_this<Connection> {
public:
    static std::shared_ptr<Connection> construct(boost::asio::io_context& ioContext, RequestHandler& handler)
    {
        return std::shared_ptr<Connection>{new Connection(ioContext, handler)};
    }

    void readRequest();

    tcp::socket& getSocket() { return socket; }

private:
    tcp::socket socket;
    boost::beast::flat_buffer buffer{};
    http::request<http::string_body> request{};
    RequestHandler& handler;

    explicit Connection(boost::asio::io_context& ioContext, RequestHandler& handler)
        : socket{ioContext}, handler{handler} { }

    void handleRequest();

    void close();
};

export class Server final : public std::enable_shared_from_this<Server> {
public:
    static std::shared_ptr<Server> construct(const std::string_view host, const unsigned short port, RequestHandler handler)
    {
        return std::shared_ptr<Server>{new Server{host, port, std::move(handler)}};
    }

    void handleRequests();

private:
    boost::asio::io_context ioContext;
    tcp::acceptor acceptor{ioContext};
    RequestHandler handler;

    Server(std::string_view host, unsigned short port, RequestHandler handler);

    void accept();
};

// FIXME: Separate this app-specific code from the generic code above.
export class Handler final {
public:
    explicit Handler(woof::Server&& server): server{server}, router{makeRouter()} { }

    http::response<http::string_body> handleRequest(const http::request<http::string_body>&);

private:
    auto makeRouter()
        -> std::unique_ptr<Matcher<http::response<http::string_body>, const http::request<http::string_body>&>>;

    http::response<http::string_body> getHealth(const http::request<http::string_body>&);
    http::response<http::string_body> postJob(const http::request<http::string_body>&);
    http::response<http::string_body> getJob(woof::JobId, const http::request<http::string_body>&);

    woof::Server server;
    std::unique_ptr<Matcher<http::response<http::string_body>, const http::request<http::string_body>&>> router;
};
};
