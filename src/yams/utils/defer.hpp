#pragma once

#define y_DEFER_UNIQUE_NAME_INNER(a, b) a##b
#define y_DEFER_UNIQUE_NAME(base, line) y_DEFER_UNIQUE_NAME_INNER(base, line)
#define y_DEFER_NAME                    y_DEFER_UNIQUE_NAME(zz_defer, __LINE__)

#define defer auto y_DEFER_NAME = details::Defer_void{} *[&]()

namespace yams {
namespace details {
template <typename Lambda> struct Deferrer {
	Lambda lambda;

	~Deferrer() {
		lambda();
	};
};

struct Defer_void {};

template <typename Lambda>
Deferrer<Lambda> operator*(Defer_void, Lambda &&lambda) {
	return {lambda};
}

} // namespace details
} // namespace yams
