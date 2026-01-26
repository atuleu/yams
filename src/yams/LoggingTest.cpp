#include <gtest/gtest.h>

class LoggingTest : public testing::Test {};

TEST_F(LoggingTest, WillFail) {
	EXPECT_TRUE(false);
}
