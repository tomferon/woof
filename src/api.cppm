module;

#include <string>
#include <nlohmann/json.hpp>

export module woof.api;

import woof.jobs;

namespace woof::api {
export struct JobSpecTo {
    std::string type;
    std::string runSpec;
    std::vector<std::string> dependencies;

    explicit JobSpecTo(std::string, std::string, std::vector<std::string>  = {});

    explicit JobSpecTo(const nlohmann::json&);

    [[nodiscard]] nlohmann::json toJson() const;
};

export struct JobSpecFrom {
    std::string type;
    std::string runSpec;
    std::unordered_map<std::string, JobId> dependencies;

    explicit JobSpecFrom(const nlohmann::json&);

    explicit JobSpecFrom(const JobSpec&);

    [[nodiscard]] JobSpec toJobSpec() const;

    [[nodiscard]] nlohmann::json toJson() const;
};

export struct JobFrom {
    JobSpec spec;

    explicit JobFrom(const Job&);

    explicit JobFrom(const nlohmann::json&);

    [[nodiscard]] Job toJob() const;

    [[nodiscard]] nlohmann::json toJson() const;
};

export struct JobSetFrom {
    std::unordered_map<std::string, JobFrom> jobs{};

    explicit JobSetFrom(const nlohmann::json&);

    explicit JobSetFrom(const std::map<JobId, const Job&>&);

    [[nodiscard]] nlohmann::json toJson() const;

    [[nodiscard]] std::map<JobId, Job> toJobs() const;
};

export struct JobSetTo {
    std::unordered_map<std::string, JobSpecTo> jobSpecs{};

    explicit JobSetTo(const nlohmann::json&);

    explicit JobSetTo(const std::unordered_map<std::string, JobSpecTo>&);

    [[nodiscard]] nlohmann::json toJson() const;

    [[nodiscard]] std::unordered_map<std::string, JobSpec> toJobSpecs() const;
};

export struct IncompleteJobSet final : std::exception {
    std::string jobName;

    explicit IncompleteJobSet(const std::string_view jobName) : jobName(std::string{jobName}) { }

    [[nodiscard]] const char* what() const noexcept override
    {
        return jobName.c_str();
    }
};

export struct CycleInJobSet final : std::exception {
    std::vector<std::string> jobNames;
    std::string message;

    explicit CycleInJobSet(std::vector<std::string> jobNames)
        : jobNames{jobNames},
          message{std::format("Cycle in job set: {}", jobNames)}
    { }

    [[nodiscard]] const char* what() const noexcept override
    {
        return message.c_str();
    }
};
}
