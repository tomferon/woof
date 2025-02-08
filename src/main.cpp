#include <memory>

import woof.http_server;
import woof.server;

int main(int argc, char** argv)
{
    woof::http_server::Handler handler{woof::Server{}};
    const auto server = woof::http_server::Server::construct(
        "127.0.0.1",
        8080,
        [&handler](auto& request) { return handler.handleRequest(request); }
    );
    server->handleRequests();
}
