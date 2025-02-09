module;

#include <boost/program_options.hpp>

export module woof.cli_options;

namespace woof {
namespace po = boost::program_options;

export struct CommonCliOptions { };

export struct ServerOptions {
    std::string host;
    std::uint16_t port;
};

export using Subcommand = std::variant<ServerOptions>;

export struct CliOptions {
    CommonCliOptions common;
    Subcommand subcommand;
};

export std::optional<CliOptions> parseCliOptions(int argc, char** argv);
export std::optional<ServerOptions> parseServerCliOptions(const std::vector<std::string>&);
}
