module;

#include <string>

export module woof.jobs;

namespace woof {
export struct JobSpec final {
	std::string type;
	std::string runSpec;

	[[nodiscard]] std::uint64_t getId() const;
};
}
