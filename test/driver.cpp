module;

#include <cstdint>
#include <iostream>
#include <ranges>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

module woof.tests.driver;

import woof.api;

namespace http = boost::beast::http;

using namespace woof::tests;

woof::JobId TestServer::submitJob(const api::JobSpecTo& jobSpec)
{
    const std::unordered_map<std::string, api::JobSpecTo> specs{{"theJob", jobSpec}};
    return submitJobs(specs).at("theJob");
}

std::unordered_map<std::string, woof::JobId> TestServer::submitJobs(const std::unordered_map<std::string, api::JobSpecTo>& jobSpecs)
{
    const http::request<http::string_body> request{
        http::verb::post,
        "/jobs",
        true,
        api::JobSetTo{jobSpecs}.toJson().dump()
    };
    auto response = httpServer.handleRequest(request);
    REQUIRE(response.result() == http::status::created);
    auto jsonBody = nlohmann::json::parse(response.body());
    return jsonBody.at("jobIds").get<std::unordered_map<std::string, std::string>>()
        | std::views::transform([](const auto& pair) {
            auto [name, jobIdStr] = pair;
            return std::pair{name, JobId{jobIdStr}};
        })
        | std::ranges::to<std::unordered_map<std::string, JobId>>();
}

std::map<woof::JobId, woof::Job> TestServer::getJobRequest(const JobId jobId, const bool closure)
{
    std::string path = std::format("/jobs/{}", jobId);
    if (closure) { path += "?closure=true"; }
    const http::request<http::string_body> request{http::verb::get, path, true};
    const auto response = httpServer.handleRequest(request);
    if (response.result() == http::status::not_found) {
        throw JobNotFound{jobId};
    }
    REQUIRE(response.result() == http::status::ok);
    return api::JobSetFrom{nlohmann::json::parse(response.body())}.toJobs();
}

std::map<woof::JobId, woof::Job> TestServer::getJobClosure(const JobId jobId)
{
    return getJobRequest(jobId, true);
}

woof::Job TestServer::getJob(const JobId jobId)
{
    return getJobRequest(jobId, false).at(jobId);
}
