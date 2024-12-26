#include <memory>

import woof.http_server;

int main(int argc, char** argv)
{
    const auto server = woof::http_server::Server::construct("127.0.0.1", 8080);
    server->handleRequests();
}
