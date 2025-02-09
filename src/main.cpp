#include <memory>
#include <variant>
#include <string>

import woof.cli_options;
import woof.http_server;
import woof.server;

int main(int argc, char** argv)
{
    const auto oopts{woof::parseCliOptions(argc, argv)};
    if (!oopts.has_value()) { return 1; }
    const auto opts = oopts.value();

    std::visit(
        [](const woof::ServerOptions& serverOpts) {
            woof::http_server::Handler handler{woof::Server{}};
            const auto server = woof::http_server::Server::construct(
                std::string_view{serverOpts.host},
                serverOpts.port,
                [&handler](auto& request) { return handler.handleRequest(request); }
            );
            server->handleRequests();
        },
        opts.subcommand
    );
}
