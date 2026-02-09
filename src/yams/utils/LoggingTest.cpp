#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <gmock/gmock.h>

#include <memory>

#include <qdebug.h>
#include <qglobal.h>
#include <slog++/Formatters.hpp>
#include <slog++/Level.hpp>
#include <slog++/Logger.hpp>
#include <slog++/MatcherTest.hpp>
#include <slog++/MockSink.hpp>
#include <slog++/slog++.hpp>

#include <yams/utils/Logging.hpp>

// Custom printers for Google Mock/Test
// These functions are discovered via ADL (Argument Dependent Lookup) and allow
// Google Test to print slog::Record objects when test expectations fail,
// making debugging much easier.
namespace slog {

/// Print a slog::Record to an ostream using slog++'s RecordToRawText formatter
void PrintTo(const Record &record, std::ostream *os) {
	Buffer buffer;
	RecordToRawText(record, buffer);
	*os << buffer;
}

/// Print a slog::Record pointer to an ostream
void PrintTo(const Record *record, std::ostream *os) {
	if (record == nullptr) {
		*os << "nullptr";
		return;
	}
	PrintTo(*record, os);
}

template <typename T, typename... Attributes>
auto ContainsAttributes(Attributes &&...attributes) {
	using ::testing::IsSupersetOf;
	std::initializer_list<Attribute> attrs = {
	    static_cast<Attribute>(attributes)...,
	};
	return VariantWith<T>(
	    Pointee(Field("attributes", &Record::attributes, IsSupersetOf(attrs)))
	);
}

/// Print a slog::Sink::RecordVariant to an ostream
/// RecordVariant is a std::variant<const Record*, unique_ptr<const Record>,
/// shared_ptr<const Record>>
void PrintTo(const Sink::RecordVariant &recordVariant, std::ostream *os) {
	std::visit(
	    [os](const auto &recordPtr) {
		    // recordPtr can be const Record*, unique_ptr, or shared_ptr
		    // All can be dereferenced with *recordPtr to get the Record
		    if constexpr (std::is_pointer_v<
		                      std::decay_t<decltype(recordPtr)>>) {
			    // Handle raw pointer case
			    PrintTo(recordPtr, os);
		    } else {
			    // Handle smart pointer cases (unique_ptr, shared_ptr)
			    if (recordPtr) {
				    PrintTo(*recordPtr, os);
			    } else {
				    *os << "nullptr";
			    }
		    }
	    },
	    recordVariant
	);
}

} // namespace slog

namespace yams {
class LoggingTest : public ::testing::Test {
protected:
	// Static members - shared across all tests in this suite
	std::shared_ptr<slog::Sink> d_originalSink;

	// Per-test members - fresh for each test
	std::shared_ptr<::testing::StrictMock<slog::MockSink>> mockSink;

	static QtMessageHandler s_originalHandler;

	static void SetUpTestSuite() {
		s_originalHandler = initLogging();
	}

	static void TearDownTestSuite() {
		qInstallMessageHandler(s_originalHandler);
	}

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

	template <typename Str, typename... Attrs>
	inline void ExpectLog(slog::Level level, Str &&message, Attrs &&...attrs) {
		using ::testing::AllOf;
		using ::testing::InSequence;
		using ::testing::Return;
		InSequence seq;
		EXPECT_CALL(*mockSink, Enabled(level)).WillOnce(Return(true));
		EXPECT_CALL(*mockSink, AllocateOnStack()).WillOnce(Return(true));
		EXPECT_CALL(
		    *mockSink,
		    Log(AllOf(
		        slog::HasLevel<const slog::Record *>(level),
		        slog::HasMessage<const slog::Record *>(std::forward<Str>(message
		        )),
		        slog::ContainsAttributes<const slog::Record *>(
		            std::forward<Attrs>(attrs)...
		        )
		    ))
		);
	}
};

QtMessageHandler LoggingTest::s_originalHandler;

TEST_F(LoggingTest, SlogInfoIsHooked) {
	using ::testing::_;
	using ::testing::Return;
	ExpectLog(slog::Level::Info, "hooked", slog::Int("answer", 42));
	// Execute: call slog::Info
	slog::Info("hooked", slog::Int("answer", 42));
}

TEST_F(LoggingTest, QtLoggingIsHooked) {
	ExpectLog(
	    slog::Level::Debug,
	    "debug is hooked",
#ifndef NDEBUG
	    slog::Int("line", 160),
#else
	    slog::String("file", ""),
	    slog::Int("line", 0),
#endif
	    slog::String("qt_category", "default")
	);
	qDebug() << "debug is hooked";

	ExpectLog(
	    slog::Level::Info,
	    "info is hooked",
#ifndef NDEBUG
	    slog::Int("line", 173),
#else
	    slog::String("file", ""),
	    slog::Int("line", 0),
#endif
	    slog::String("qt_category", "default")
	);
	qInfo() << "info is hooked";

	ExpectLog(
	    slog::Level::Warn,
	    "warning is hooked",
#ifndef NDEBUG
	    slog::Int("line", 186),
#else
	    slog::String("file", ""),
	    slog::Int("line", 0),
#endif
	    slog::String("qt_category", "default")
	);
	qWarning() << "warning is hooked";

	ExpectLog(
	    slog::Level::Error,
	    "critical is hooked",
#ifndef NDEBUG
	    slog::Int("line", 199),
#else
	    slog::String("file", ""),
	    slog::Int("line", 0),
#endif
	    slog::String("qt_category", "default")
	);
	qCritical() << "critical is hooked";
}

TEST_F(LoggingTest, FatalLogging) {
	// TODO: check that fatal is hooked, but it is too complex to handle.
}

} // namespace yams
