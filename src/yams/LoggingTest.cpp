#include "gmock/gmock.h"
#include <gmock/gmock.h>

#include <iterator>
#include <memory>

#include <slog++/MatcherTest.hpp>
#include <slog++/MockSink.hpp>
#include <slog++/slog++.hpp>

class LoggingTest : public ::testing::Test {
protected:
	// Static members - shared across all tests in this suite
	std::shared_ptr<slog::Sink> d_originalSink;

	// Per-test members - fresh for each test
	std::shared_ptr<::testing::StrictMock<slog::MockSink>> mockSink;

	// Suite-level setup: save original sink
	// Per-test setup: install fresh MockSink
	void SetUp() override {
		// Create a fresh StrictMock for each test
		mockSink = std::make_shared<::testing::StrictMock<slog::MockSink>>();

		// Install it on the default logger
		d_originalSink = slog::DefaultLogger().SetSink(mockSink);
	}

	// Per-test teardown: restore original sink
	void TearDown() override {
		// Restore original sink for other tests
		if (d_originalSink) {
			slog::DefaultLogger().SetSink(std::move(d_originalSink));
		}
		mockSink.reset();
	}
};

template <typename Str, typename... Attrs>

void ExpectLog(
    ::testing::StrictMock<slog::MockSink> &mock,
    slog::Level                            level,
    Str                                  &&message,
    Attrs &&...attrs
) {
	using ::testing::AllOf;
	using ::testing::InSequence;
	using ::testing::Return;
	InSequence seq;
	EXPECT_CALL(mock, Enabled(level)).WillOnce(Return(true));
	EXPECT_CALL(mock, AllocateOnStack()).WillOnce(Return(true));
	EXPECT_CALL(
	    mock,
	    Log(AllOf(
	        slog::HasLevel<const slog::Record *>(level),
	        slog::HasMessage<const slog::Record *>(std::forward<Str>(message)),
	        slog::HasAttributes<const slog::Record *>(std::forward<Attrs>(attrs
	        )...)
	    ))
	);
}

TEST_F(LoggingTest, SlogInfoIsHooked) {
	using ::testing::_;
	using ::testing::Return;
	ExpectLog(*mockSink, slog::Level::Info, "hooked", slog::Int("answer", 42));
	// Execute: call slog::Info
	slog::Info("hooked", slog::Int("answer", 42));
}
