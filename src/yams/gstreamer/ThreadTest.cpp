#include "Thread.hpp"

#include <QApplication>
#include <future>
#include <glib.h>
#include <gmock/gmock.h>
#include <gst/gstbus.h>
#include <gst/gstmessage.h>
#include <gst/gstobject.h>
#include <gst/gststructure.h>
#include <gtest/gtest.h>
#include <memory>
#include <qcoreapplication.h>
#include <qnamespace.h>
#include <qtypes.h>

#include <slog++/slog++.hpp>

namespace yams {
class GstThreadTest : public testing::Test {
protected:
	GstThread *thread;
	GstBusPtr  bus;

	void SetUp() override {
		thread = new GstThread{nullptr};
		thread->start();
		bus = GstBusPtr{gst_bus_new()};
	}

	void TearDown() override {
		thread->quit();
		thread->wait();
		delete thread;
		bus.reset();
	}
};

struct CallerData {
	QThread          *caller{nullptr};
	std::atomic<bool> set{false};

	void fromCurrent() {
		caller = QThread::currentThread();

		slog::Debug("caller", slog::Pointer("address", caller));
		set.store(true);
		set.notify_all();
	}

	void wait() {
		set.wait(false);
	};
};

static gboolean
getCallerMessage(GstBus *bus, GstMessage *msg, gpointer userdata) {
	auto callerData = reinterpret_cast<CallerData *>(userdata);
	callerData->fromCurrent();
	return false;
}

TEST_F(GstThreadTest, MessageRunInTheThread) {
	using namespace std::chrono_literals;
	CallerData callerData;
	thread->watchBus(bus.get(), getCallerMessage, &callerData);
	auto structure = gst_structure_new_empty("utest");
	auto message =
	    gst_message_new_application(GST_OBJECT_CAST(bus.get()), structure);
	gst_bus_post(bus.get(), message);

	auto future = std::make_unique<std::future<void>>(
	    std::async(std::launch::async, [&]() { callerData.wait(); })
	);
	if (future->wait_for(2s) == std::future_status::timeout) {

		ADD_FAILURE() << "Timeouted";
		future.release(); // intentional memory leak on failure.
		return;
	}

	EXPECT_EQ(callerData.caller, thread);
}

class MockThread : public GstThreadLegacy {
public:
	MOCK_METHOD(void, startTask, (), (override));
	MOCK_METHOD(void, doneTask, (), (override));
};

class GstThreadLegacyTest : public testing::Test {
protected:
	MockThread d_thread;

	void TearDown() {
		EXPECT_CALL(d_thread, doneTask()).Times(1);
		d_thread.stop();
		EXPECT_TRUE(d_thread.wait(1000));
	}
};

TEST_F(GstThreadLegacyTest, RunGLibSourceInRightThread) {
	std::atomic<const char *> threadName{nullptr};

	EXPECT_CALL(d_thread, startTask()).Times(1).WillOnce([&]() {
		auto context = g_main_context_get_thread_default();
		g_main_context_invoke(
		    context,
		    [](gpointer userdata) -> gboolean {
			    auto threadName =
			        reinterpret_cast<std::atomic<const char *> *>(userdata);
			    static std::string result =
			        QThread::currentThread()->objectName().toStdString();

			    threadName->store(result.c_str());
			    threadName->notify_all();
			    return false;
		    },
		    reinterpret_cast<gpointer>(&threadName)
		);
	});

	d_thread.start();
	d_thread.setObjectName("glibThread");
	ASSERT_STREQ(d_thread.objectName().toStdString().c_str(), "glibThread");
	using namespace std::chrono_literals;
	// wait with a timeout of 2s for called to become true
	auto future = std::make_unique<std::future<void>>(

	    std::async(std::launch::async, [&]() { threadName.wait(nullptr); })
	);
	if (future->wait_for(2s) == std::future_status::timeout) {
		ADD_FAILURE() << "Timeouted";
		future.reset(); // intentional leak;
	} else {
		EXPECT_STREQ(threadName.load(), "glibThread");
	}
}

} // namespace yams
