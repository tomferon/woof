module;

#include <cstdint>
#include <exception>
#include <map>
#include <string>

export module woof.engine;

import woof.jobs;

namespace woof {
export struct JobNotFound final : std::exception {
    JobId jobId;
    std::string jobIdStr;

    explicit JobNotFound(const JobId jobId)
        : jobId(jobId), jobIdStr{jobId.toString()}
    { }

    [[nodiscard]] const char* what() const noexcept override
    {
        return jobIdStr.c_str();
    }
};

export class Engine final {
public:
    JobId addJob(const JobSpec&);

    [[nodiscard]] const Job& getJob(JobId) const;

    [[nodiscard]] std::map<JobId, const Job&> getJobs(JobId) const;

private:
    std::map<JobId, Job> jobs;
};
}
