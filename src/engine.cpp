module;

#include <queue>
#include <utility>

module woof.engine;

woof::JobId woof::Engine::addJob(const JobSpec& jobSpec)
{
    const JobId jobId = jobSpec.getId();
    jobs.emplace(jobId, jobSpec);
    return jobId;
}

const woof::Job& woof::Engine::getJob(const JobId jobId) const
{
    if (const auto it = jobs.find(jobId); it == jobs.end()) {
        throw JobNotFound{jobId};
    } else {
        return it->second;
    }
}

std::map<woof::JobId, const woof::Job&> woof::Engine::getJobs(const JobId rootJobId) const
{
    std::map<JobId, const Job&> jobs;
    std::deque pendingJobIds{{rootJobId}};
    while (!pendingJobIds.empty()) {
        if (const JobId jobId = pendingJobIds.front(); !jobs.contains(jobId)) {
            const Job& job = getJob(jobId);
            for (const auto& [_, depJobId]: job.spec.dependencies) {
                pendingJobIds.push_back(depJobId);
            }
            jobs.emplace(jobId, job);
        }
        pendingJobIds.pop_front();
    }
    return jobs;
}
