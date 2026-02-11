#pragma once

#include <cmath>
#include <tuple>
#include <type_traits>

namespace yams {
namespace details {
template <typename Integer> constexpr Integer gcd(Integer a, Integer b) {
	static_assert(std::is_integral_v<Integer>, "must be integral");

	if (a == 0) {
		return b;
	}
	if (b == 0) {
		return a;
	}
	if (a < b) {
		return gcd(a, Integer(b % a));
	} else {
		return gcd(b, Integer(a % b));
	}
}

} // namespace details

template <typename Floating, typename Integer = int64_t>
constexpr std::tuple<Integer, Integer>
build_fraction(Floating f, Floating eps = 1.0e-6) {
	static_assert(std::is_integral_v<Integer>, "Integer must be integral");
	static_assert(
	    std::is_floating_point_v<Floating>,
	    "Floating must be integral"
	);

	Floating      integral  = std::floor(f);
	Floating      rem       = f - integral;
	const Integer precision = 1000000LL;
	Integer gcd = details::gcd(Integer(std::round(rem * precision)), precision);

	long denominator = precision / gcd;
	long numerator   = std::round(rem * precision) / gcd;
	return {integral * denominator + numerator, denominator};
}

} // namespace yams
