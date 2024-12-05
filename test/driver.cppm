module;

#include <cstdint>

#include <catch2/matchers/catch_matchers_templated.hpp>

export module woof.tests.driver;

import woof.engine;
import woof.jobs;

namespace woof::tests {
export class Server {
public:
	std::uint64_t submitJob(const JobSpec&);
	[[nodiscard]] JobSpec getJob(std::uint64_t jobId) const;

private:
	Engine engine{};
};

export class JobNotFoundMatcher : public Catch::Matchers::MatcherGenericBase {
public:
	explicit JobNotFoundMatcher(const std::uint64_t jobId): jobId{jobId} {}

	[[nodiscard]] bool match(const JobNotFound& exc) const
	{
		return exc.jobId == jobId;
	}

	std::string describe() const override
	{
		return std::format("Is a JobNotFound exception with jobId = {}", jobId);
	}

private:
	std::uint64_t jobId;
};

export JobNotFoundMatcher isJobNotFound(const std::uint64_t jobId)
{
	return JobNotFoundMatcher{jobId};
}
}
