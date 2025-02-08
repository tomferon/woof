module;

#include <iostream>
#include <ranges>
#include <unordered_set>
#include <nlohmann/json.hpp>

module woof.api;

import woof.jobs;

using namespace woof::api;

JobSpecTo::JobSpecTo(std::string type, std::string runSpec, std::vector<std::string> dependencies)
    : type{std::move(type)}, runSpec{std::move(runSpec)}, dependencies{std::move(dependencies)}
{ }

JobSpecTo::JobSpecTo(const nlohmann::json& json)
    : type{json.at("type").get<std::string>()},
      runSpec{json.at("runSpec").get<std::string>()},
      dependencies{json.at("dependencies").get<std::vector<std::string>>()}
{ }

nlohmann::json JobSpecTo::toJson() const
{
    nlohmann::json depsJson = nlohmann::json::array();
    std::ranges::copy(dependencies, std::back_inserter(depsJson));
    return {
        {"type", type},
        {"runSpec", runSpec},
        {"dependencies", depsJson},
    };
}

JobSpecFrom::JobSpecFrom(const nlohmann::json& json)
    : type{json.at("type").get<std::string>()},
      runSpec{json.at("runSpec").get<std::string>()}
{
    dependencies = json.at("dependencies").get<std::unordered_map<std::string, nlohmann::json>>()
        | std::views::transform([](const std::pair<std::string, nlohmann::json>& pair) {
            auto [name, idJson] = pair;
            return std::pair{name, JobId{idJson.get<std::string>()}};
        })
        | std::ranges::to<std::unordered_map<std::string, JobId>>();
}

nlohmann::json JobSpecFrom::toJson() const
{
    nlohmann::json depsJson = nlohmann::json::object();
    for (const auto& [name, id]: dependencies) {
        depsJson[name] = id.toString();
    }
    return {
        {"type", type},
        {"runSpec", runSpec},
        {"dependencies", depsJson},
    };
}

JobSpecFrom::JobSpecFrom(const JobSpec& jobSpec)
    : type{jobSpec.type}, runSpec{jobSpec.runSpec}, dependencies{jobSpec.dependencies}
{ }

JobFrom::JobFrom(const Job& job)
    : spec{job.spec}
{ }

woof::JobSpec JobSpecFrom::toJobSpec() const
{
    return {type, runSpec, dependencies};
}

JobFrom::JobFrom(const nlohmann::json& json)
    : spec{JobSpecFrom{json.at("spec")}.toJobSpec()}
{ }

woof::Job JobFrom::toJob() const
{
    return Job{spec};
}

nlohmann::json JobFrom::toJson() const
{
    return {
        {"spec", JobSpecFrom{spec}.toJson()}
    };
}

JobSetFrom::JobSetFrom(const nlohmann::json& json)
{
    const auto jobList = json.at("jobs").get<std::map<std::string, nlohmann::json>>();
    for (const auto& [name, jobJson]: jobList) {
        jobs.emplace(std::string{name}, jobJson);
    }
}

JobSetFrom::JobSetFrom(const std::map<JobId, const Job&>& jobList)
{
    for (const auto& [id, job] : jobList) {
        jobs.emplace(id.toString(), JobFrom{job});
    }
}

nlohmann::json JobSetFrom::toJson() const
{
    nlohmann::json jobList = nlohmann::json::object();
    for (const auto& [name, job]: jobs) {
        jobList[name] = job.toJson();
    }
    return {
        {"jobs", jobList}
    };
}

std::map<woof::JobId, woof::Job> JobSetFrom::toJobs() const
{
    return jobs
        | std::views::transform([](const auto& pair) {
            const auto& [jobIdStr, jobFrom] = pair;
            return std::pair<JobId, Job>{JobId{jobIdStr}, jobFrom.toJob()};
        })
        | std::ranges::to<std::map<JobId, Job>>();
}

JobSetTo::JobSetTo(const nlohmann::json& json)
{
    const auto specList = json.at("jobSpecs").get<std::unordered_map<std::string_view, nlohmann::json>>();
    for (const auto& [name, specJson]: specList) {
        jobSpecs.emplace(std::string{name}, specJson);
    }
}

JobSetTo::JobSetTo(const std::unordered_map<std::string, JobSpecTo>& specs)
    : jobSpecs{specs}
{ }

nlohmann::json JobSetTo::toJson() const
{
    nlohmann::json specList = nlohmann::json::object();
    for (const auto& [name, spec]: jobSpecs) {
        specList[name] = spec.toJson();
    }
    return {
        {"jobSpecs", specList}
    };
}

std::unordered_map<std::string, woof::JobSpec> JobSetTo::toJobSpecs() const
{
    std::unordered_map<std::string, JobSpec> result{};

    std::vector<std::pair<std::string, JobSpecTo>> queue{};
    std::vector<std::pair<std::string, JobSpecTo>> nextQueue{};
    queue.reserve(jobSpecs.size());
    std::ranges::copy(jobSpecs, std::back_inserter(queue));

    for (; !queue.empty(); queue = std::move(nextQueue), nextQueue.clear()) {
        bool someJobResolved = false;
        std::unordered_map<std::string_view, std::string_view> failedNames{};

        for (const auto& [name, specTo]: queue) {
            bool someDepUnresolved = false;
            std::unordered_map<std::string, JobId> deps{};
            for (const std::string& depName : specTo.dependencies) {
                if (const auto it = result.find(depName); it != result.end()) {
                    deps.emplace(depName, it->second.getId());
                } else {
                    someDepUnresolved = true;
                    failedNames.emplace(name, depName);
                    break;
                }
            }
            if (!someDepUnresolved) {
                JobSpec spec{specTo.type, specTo.runSpec, deps};
                someJobResolved = true;
                result.emplace(name, spec);
            } else {
                nextQueue.emplace_back(name, specTo);
            }
        }

        if (!someJobResolved) {
            for (const auto& pair : failedNames) {
                if (!failedNames.contains(pair.second)) {
                    throw IncompleteJobSet{pair.second};
                }
            }
            // If the previous loop has not thrown, then it must have failed due a cycle.
            std::string_view current{failedNames.begin()->first};
            std::vector<std::string_view> path{current};
            while (true) {
                std::string_view next = failedNames.at(current);
                if (const auto cycleStart{std::ranges::find(path, next)}; cycleStart == path.end()) {
                    path.emplace_back(next);
                    current = next;
                } else {
                    const auto cycle{std::vector<std::string>{cycleStart, path.end()}};
                    throw CycleInJobSet{cycle};
                }
            }
        }
    }

    return result;
}
