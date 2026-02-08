#include "Version.hpp"
#include "git.h"

namespace yams {
std::string Version() {
	if (git::AnyUncommittedChanges()) {
		return std::string{git::Describe()} + "-dirty";
	} else {
		return std::string{git::Describe()};
	}
}
} // namespace yams
