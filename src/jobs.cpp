module;

#include <format>
#include <ios>
#include <sstream>
#include <string>
#include <vector>
#include <__algorithm/ranges_sort.h>

module woof.jobs;

woof::JobId::JobId(std::string str)
{
    std::stringstream ss{str};
    ss >> std::hex >> id;
}

std::string woof::JobId::toString() const
{
    return std::format("{:0x}", id);
}

std::istream& operator>>(std::istream& is, woof::JobId& jobId)
{
    std::string input;
    is >> input;
    jobId = woof::JobId{input};
    return is;
}

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

        std::vector<std::pair<std::string, woof::JobId>> depsList{};
        depsList.reserve(jobSpec.dependencies.size());
        std::ranges::copy(jobSpec.dependencies, std::back_inserter(depsList));
        std::ranges::sort(depsList, {}, &std::pair<std::string, woof::JobId>::first);
        for (const auto& [name, jobId] : depsList) {
            result = result ^ (stringHash(name) * prime);
            result = result ^ (jobId.id * prime);
        }

        return result;
    }
};

woof::JobId woof::JobSpec::getId() const
{
    constexpr std::hash<JobSpec> jobSpecHash{};
    return JobId{jobSpecHash(*this)};
}

