module;

#include <string>

module woof.jobs;

template<>
struct std::hash<woof::JobSpec> {
    // Hash the different fields of the job spec using FNV-1a.
    std::uint64_t operator()(const woof::JobSpec& jobSpec) const noexcept
    {
        constexpr std::uint64_t prime{0x100000001B3};
        std::uint64_t result{0xcbf29ce484222325}; // offset

        constexpr std::hash<std::string> stringHash{};

        result = result ^ (stringHash(jobSpec.type) * prime);
        result = result ^ (stringHash(jobSpec.runSpec) * prime);

        return result;
    }
};

std::uint64_t woof::JobSpec::getId() const
{
    constexpr std::hash<JobSpec> jobSpecHash{};
    return jobSpecHash(*this);
}
