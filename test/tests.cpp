#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>
#include <ranges>

import woof.api;
import woof.tests.driver;
import woof.engine;
import woof.jobs;

SCENARIO("Submitting jobs")
{
    woof::tests::TestServer server{};

    WHEN("a request with no jobs is submitted") {
        const auto jobIds = server.submitJobs({});

        THEN("the response is an empty set of job IDs") {
            REQUIRE(jobIds.empty());
        }
    }

    GIVEN("a single job specification") {
        const auto jobSpec = woof::api::JobSpecTo{"command", "echo Hello"};

        WHEN("the job is submitted and then retrieved") {
            const auto jobId = server.submitJob(jobSpec);
            const auto [spec] = server.getJob(jobId);
            const auto [type, runSpec, _] = spec;

            THEN("they should be equal") {
                CHECK_THAT(type, Catch::Matchers::Equals(jobSpec.type));
                CHECK_THAT(runSpec, Catch::Matchers::Equals(jobSpec.runSpec));
            }
        }

        WHEN("the job is not sent first") {
            THEN("retrieval throws woof::JobNotFound") {
                const woof::JobId jobId{345678};
                REQUIRE_THROWS_MATCHES(server.getJob(jobId), woof::JobNotFound, woof::tests::isJobNotFound(jobId));
            }
        }
    }

    GIVEN("multiple job specifications") {
        const auto jobSpec1 = woof::api::JobSpecTo{"command", "echo Hello"};
        const auto jobSpec2 = woof::api::JobSpecTo{"command", "echo World"};

        WHEN("the jobs are submitted and then retrieved") {
            const auto jobIds = server.submitJobs({{"job1", jobSpec1}, {"job2", jobSpec2}});
            const auto [spec1] = server.getJob(jobIds.at("job1"));
            const auto [type1, runSpec1, deps1] = spec1;
            const auto [spec2] = server.getJob(jobIds.at("job2"));
            const auto [type2, runSpec2, deps2] = spec2;

            THEN("they should be equal") {
                CHECK_THAT(type1, Catch::Matchers::Equals(jobSpec1.type));
                CHECK_THAT(runSpec1, Catch::Matchers::Equals(jobSpec1.runSpec));
                CHECK(deps1.empty());
                CHECK_THAT(type2, Catch::Matchers::Equals(jobSpec2.type));
                CHECK_THAT(runSpec2, Catch::Matchers::Equals(jobSpec2.runSpec));
                CHECK(deps2.empty());
            }
        }
    }

    GIVEN("interdependent jobs") {
        const woof::api::JobSpecTo childJob1{"command", "echo Child1"};
        const woof::api::JobSpecTo childJob2{"command", "echo Child2"};
        const woof::api::JobSpecTo intermediateJob{"command", "echo Intermediate", {"child1", "child2"}};
        const woof::api::JobSpecTo rootJob{"command", "echo Root", {"intermediate"}};
        const woof::api::JobSpecTo unrelatedJob{"command", "echo Unrelated"};

        const auto result = server.submitJobs({
            {"root", rootJob},
            {"intermediate", intermediateJob},
            {"child1", childJob1},
            {"child2", childJob2},
        });
        const woof::JobId rootId = result.at("root");

        WHEN("the root job is retrieved with closure=false") {
            const auto jobList = server.getJobRequest(rootId, false);

            THEN("we only get the root job back") {
                REQUIRE(jobList.size() == 1);
                const auto [spec] = jobList.at(rootId);
                CHECK_THAT(spec.type, Catch::Matchers::Equals(rootJob.type));
                CHECK_THAT(spec.runSpec, Catch::Matchers::Equals(rootJob.runSpec));
            }
        }

        WHEN("the root job is retrieved with closure=true") {
            const auto jobList = server.getJobRequest(rootId, true);

            THEN("we get all jobs in its transitive closure back")
            {
                const auto jobIds = jobList
                    | std::views::keys
                    | std::ranges::to<std::vector<woof::JobId>>();
                const auto expectedIds = result
                    | std::views::values
                    | std::ranges::to<std::vector<woof::JobId>>();
                CHECK_THAT(jobIds, Catch::Matchers::UnorderedEquals(expectedIds));
            }
        }
    }

    GIVEN("an incomplete set of jobs")
    {
        const woof::api::JobSpecTo childJob1{"command", "echo Child1"};
        const woof::api::JobSpecTo intermediateJob{"command", "echo Intermediate", {"child1", "child2"}};
        const woof::api::JobSpecTo rootJob{"command", "echo Root", {"intermediate"}};

        THEN("submitting them returns an error")
        {
            REQUIRE_THROWS_MATCHES(
                server.submitJobs({
                    {"root", rootJob},
                    {"intermediate", intermediateJob},
                    {"child1", childJob1},
                }),
                woof::api::IncompleteJobSet,
                woof::tests::isIncompleteJobSet("child2")
            );
        }
    }

    GIVEN("a set of jobs with a cycle")
    {
        const woof::api::JobSpecTo jobA{"command", "echo A"};
        const woof::api::JobSpecTo jobB{"command", "echo B", {"c"}};
        const woof::api::JobSpecTo jobC{"command", "echo C", {"d"}};
        const woof::api::JobSpecTo jobD{"command", "echo D", {"b"}};

        THEN("submitting them returns an error")
        {
            REQUIRE_THROWS_MATCHES(
                server.submitJobs({{"a", jobA}, {"b", jobB}, {"c", jobC}, {"d", jobD}}),
                woof::api::CycleInJobSet,
                woof::tests::isCycleInJobSet({"b", "d", "c"})
            );
        }
    }
}
