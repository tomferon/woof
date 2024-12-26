module;

#include <cstdint>

module woof.tests.driver;

std::uint64_t woof::tests::Server::submitJob(const JobSpec& jobSpec)
{
    return engine.addJob(jobSpec);
}

woof::JobSpec woof::tests::Server::getJob(const std::uint64_t jobId) const
{
    return engine.getJob(jobId);
}
