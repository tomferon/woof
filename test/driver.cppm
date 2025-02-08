module;

#include <map>
#include <unordered_map>

#include <catch2/matchers/catch_matchers_templated.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>

export module woof.tests.driver;

import woof.api;
import woof.engine;
import woof.http_server;
import woof.jobs;
import woof.server;

namespace woof::tests {
export class TestServer final {
public:
    JobId submitJob(const api::JobSpecTo&);

    std::unordered_map<std::string, JobId> submitJobs(const std::unordered_map<std::string, api::JobSpecTo>&);

    [[nodiscard]] std::map<JobId, Job> getJobRequest(JobId, bool closure);

    [[nodiscard]] Job getJob(JobId);

    [[nodiscard]] std::map<JobId, Job> getJobClosure(JobId);

private:
    http_server::Handler httpServer{Server{}};
};

export class JobNotFoundMatcher final : public Catch::Matchers::MatcherGenericBase {
public:
    explicit JobNotFoundMatcher(const JobId jobId): jobId{jobId} { }

    [[nodiscard]] bool match(const JobNotFound& exc) const
    {
        return exc.jobId == jobId;
    }

    std::string describe() const override
    {
        return std::format("Is a JobNotFound exception with jobId = {}", jobId);
    }

private:
    JobId jobId;
};

export JobNotFoundMatcher isJobNotFound(const JobId jobId)
{
    return JobNotFoundMatcher{jobId};
}

export class IncompleteJobSetMatcher final : public Catch::Matchers::MatcherGenericBase {
public:
    explicit IncompleteJobSetMatcher(const std::string_view name): jobName{std::string{name}} { }

    [[nodiscard]] bool match(const api::IncompleteJobSet& exc) const
    {
        return exc.jobName == jobName;
    }

    std::string describe() const override
    {
        return std::format("Is an IncompleteJobSetMatcher exception with jobName = {}", jobName);
    }

private:
    std::string jobName;
};

export IncompleteJobSetMatcher isIncompleteJobSet(const std::string_view jobName)
{
    return IncompleteJobSetMatcher{jobName};
}

export class CycleInJobSetMatcher final : public Catch::Matchers::MatcherGenericBase {
public:
    explicit CycleInJobSetMatcher(std::vector<std::string> jobNames): jobNames{jobNames} { }

    [[nodiscard]] bool match(const api::CycleInJobSet& exc) const
    {
        // The order is in fact important, but we don't care about which is the first in the list since it's a cycle.
        // Checking they have the same elements is good enough for testing purposes.
        return Catch::Matchers::UnorderedEquals(jobNames).match(exc.jobNames);
    }

    std::string describe() const override
    {
        return std::format("Is an CycleInJobSet exception with jobNames = {}", jobNames);
    }

private:
    std::vector<std::string> jobNames;
};

export CycleInJobSetMatcher isCycleInJobSet(std::vector<std::string> jobNames)
{
    return CycleInJobSetMatcher{std::move(jobNames)};
}
}

template<>
struct Catch::StringMaker<woof::JobId> {
    static std::string convert(woof::JobId const& jobId) {
        return jobId.toString();
    }
};