#include "fractional.hpp"
#include <gtest/gtest.h>

namespace yams {
class FractionalTest : public ::testing::Test {};

TEST_F(FractionalTest, GCD) {
	EXPECT_EQ(yams::details::gcd(true, false), true);
	EXPECT_EQ(yams::details::gcd(false, true), true);
	EXPECT_EQ(yams::details::gcd(true, true), true);
	EXPECT_EQ(yams::details::gcd(false, false), false);

	EXPECT_EQ(yams::details::gcd(1, 2), 1);
	EXPECT_EQ(yams::details::gcd(2, 3), 1);
	EXPECT_EQ(yams::details::gcd(3, 6), 3);
	EXPECT_EQ(yams::details::gcd(3 * 5, 3 * 7), 3);
}

TEST_F(FractionalTest, Fraction) {
	EXPECT_EQ(yams::build_fraction(1.0f), std::make_tuple(1, 1));
	EXPECT_EQ(yams::build_fraction(25.0f), std::make_tuple(25, 1));
	EXPECT_EQ(yams::build_fraction(33.333f), std::make_tuple(33333, 1000));
}

} // namespace yams
