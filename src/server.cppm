module;

#include <map>
#include <ranges>
#include <unordered_map>

export module woof.server;

import woof.engine;
import woof.jobs;

namespace woof {
export class Server {
public:
    // FIXME: Move definitions to server.cpp
    std::unordered_map<std::string, JobId> addJobs(const std::unordered_map<std::string, JobSpec>& jobSpecs)
    {
        return jobSpecs
            | std::views::transform([&](const auto& pair) {
                const auto& [name, jobSpec] = pair;
                const auto jobId = engine.addJob(jobSpec);
                return std::pair{name, jobId};
            })
            | std::ranges::to<std::unordered_map<std::string, JobId>>();
    }

    [[nodiscard]] std::map<JobId, const Job&> getJobs(const JobId jobId, const bool closure = false) const
    {
        if (closure) {
            return engine.getJobs(jobId);
        } else {
            const Job& job = engine.getJob(jobId);
            return std::map<JobId, const Job&>{{jobId, job}};
        }
    };

private:
    Engine engine;
};
}
