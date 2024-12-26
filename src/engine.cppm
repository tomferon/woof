module;

#include <cstdint>
#include <exception>
#include <map>
#include <string>

export module woof.engine;

import woof.jobs;

namespace woof {
export struct JobNotFound final : std::exception {
    std::uint64_t jobId;

    explicit JobNotFound(const std::uint64_t jobId)
        : jobId(jobId)
    { }

    [[nodiscard]] const char* what() const noexcept override
    {
        return std::to_string(jobId).c_str();
    }
};

export class Engine final {
public:
    std::uint64_t addJob(const JobSpec&);

    [[nodiscard]] const JobSpec& getJob(std::uint64_t jobId) const;

private:
    std::map<uint64_t, JobSpec> jobs;
};
}
