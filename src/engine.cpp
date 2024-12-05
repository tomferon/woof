module;

#include <cstdint>
#include <utility>

module woof.engine;

std::uint64_t woof::Engine::addJob(const JobSpec& jobSpec)
{
	const std::uint64_t jobId = jobSpec.getId();
	jobs.insert(std::make_pair(jobId, jobSpec));
	return jobId;
}

const woof::JobSpec& woof::Engine::getJob(const std::uint64_t jobId) const
{
	if (const auto it = jobs.find(jobId); it == jobs.end()) {
		throw JobNotFound{jobId};
	} else {
		return it->second;
	}
}
