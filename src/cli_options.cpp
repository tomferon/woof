module;

#include <print>

#include <boost/program_options.hpp>

module woof.cli_options;

std::optional<woof::CliOptions> woof::parseCliOptions(int argc, char** argv)
{
    po::options_description commonDesc{"Common options"};
    commonDesc.add_options()
        ("help", "Show help message")
        ("subcommand", po::value<std::string>(), "Subcommand to run")
        ("subargs", po::value<std::vector<std::string>>(), "Subcommand arguments");

    po::positional_options_description pos;
    pos.add("subcommand", 1).add("subargs", -1);

    po::variables_map vm;
    const auto parsedOptions =
        po::command_line_parser(argc, argv).options(commonDesc).positional(pos).allow_unregistered().run();
    store(parsedOptions, vm);
    notify(vm);

    if (vm.contains("help")) {
        std::println("FIXME: Print help");
        return {};
    }

    if (!vm.contains("subcommand")) {
        std::println("Missing subcommand. Use --help for more information.");
        return {};
    }

    const auto subcommand = vm["subcommand"].as<std::string>();
    std::vector<std::string> opts =
        collect_unrecognized(parsedOptions.options, po::include_positional);
    opts.erase(std::ranges::find(opts, subcommand));

    std::unordered_map<std::string, std::function<std::optional<Subcommand>(const std::vector<std::string>&)>> parsers{
        {"server", parseServerCliOptions}
    };
    if (const auto parser = parsers.find(subcommand); parser != parsers.end()) {
        return parser->second(opts).transform([&](const auto& scmdOptions) {
            return CliOptions{{}, scmdOptions};
        });
    }

    std::println("Invalid subcommand '{}'. Use --help for more information.", subcommand);
    return {};
}

std::optional<woof::ServerOptions> woof::parseServerCliOptions(const std::vector<std::string>& opts)
{
    po::options_description desc{"Options for subcommand 'server'"};
    desc.add_options()
        ("host", po::value<std::string>(), "Host to bind to")
        ("port", po::value<std::uint16_t>(), "Port to bind to");

    po::variables_map vm;
    store(po::command_line_parser(opts).options(desc).run(), vm);
    notify(vm);

    const auto host = vm.contains("host") ? vm["host"].as<std::string>() : "localhost";
    const auto port = vm.contains("port") ? vm["port"].as<std::uint16_t>() : std::uint16_t{3854};
    return ServerOptions{host, port};
}
