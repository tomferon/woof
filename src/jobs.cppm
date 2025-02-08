module;

#include <format>
#include <string>
#include <unordered_map>

export module woof.jobs;

namespace woof {
export struct JobId {
    std::uint64_t id{0};

    JobId() = default;

    explicit JobId(const std::uint64_t id): id{id} { };

    explicit JobId(std::string);

    [[nodiscard]] std::string toString() const;

    [[nodiscard]] std::strong_ordering operator<=>(const JobId& other) const noexcept = default;
};

export struct JobSpec final {
    std::string type;
    std::string runSpec;
    std::unordered_map<std::string, JobId> dependencies;

    [[nodiscard]] JobId getId() const;
};

export struct Job final {
    JobSpec spec;
};
}

export template<>
struct std::formatter<woof::JobId> {
    template<typename ParseContext>
    constexpr ParseContext::iterator parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template<typename FormatContext>
    FormatContext::iterator format(woof::JobId jobId, FormatContext& ctx) const
    {
        return std::ranges::copy(jobId.toString(), ctx.out()).out;
    }
};

export std::istream& operator>>(std::istream&, woof::JobId&);

