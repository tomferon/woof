#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

import woof.tests.driver;
import woof.engine;
import woof.jobs;

SCENARIO("Submitting jobs")
{
    woof::tests::Server server{};

    GIVEN("a single job specification") {
        const auto jobSpec = woof::JobSpec{"command", "echo Hello"};

        WHEN("the job is submitted and then retrieved") {
            const auto jobId = server.submitJob(jobSpec);
            const auto [type, runSpec] = server.getJob(jobId);

            THEN("they should be equal") {
                CHECK_THAT(type, Catch::Matchers::Equals(jobSpec.type));
                CHECK_THAT(runSpec, Catch::Matchers::Equals(jobSpec.runSpec));
            }
        }

        WHEN("the job is not sent first") {
            THEN("retrieval throws woof::JobNotFound") {
                const auto jobId = jobSpec.getId();
                REQUIRE_THROWS_MATCHES(server.getJob(jobId), woof::JobNotFound, woof::tests::isJobNotFound(jobId));
            }
        }
    }

    GIVEN("interdependent jobs") {
        // FIXME: implement this test
    }
}
