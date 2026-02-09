#include "gmock/gmock.h"
#include <gmock/gmock.h>
#include <gst/gstbus.h>
#include <gst/gstdeviceprovider.h>
#include <gst/gstmessage.h>
#include <gst/gstobject.h>
#include <gst/gststructure.h>
#include <yams/gstreamer/Pipeline.hpp>

namespace yams {
class MockPipeline : public yams::Pipeline {
public:
	MockPipeline()
	    : yams::Pipeline{"utest", nullptr} {}

	MOCK_METHOD(
	    GstBusSyncReply, onSyncMessage, (GstMessage *), (noexcept, override)
	);
	MOCK_METHOD(void, onMessage, (GstMessage *), (noexcept, override));

	void postMessage() {
		auto structure = gst_structure_new_empty("mockPipelineStruct");
		auto msg       = gst_message_new_application(
            GST_OBJECT_CAST(d_pipeline.get()),
            structure
        );
		gst_bus_post(d_bus.get(), msg);
	}

	GstMessage *poll() const {
		return gst_bus_pop(d_bus.get());
	}
};

class PipelineTest : public ::testing::Test {
protected:
	std::unique_ptr<QThread>                             thread;
	std::unique_ptr<::testing::StrictMock<MockPipeline>> pipeline;

	void SetUp() {
		thread   = std::make_unique<QThread>();
		pipeline = std::make_unique<::testing::StrictMock<MockPipeline>>();
		pipeline->moveToThread(thread.get());
		thread->start();
		EXPECT_NE(QThread::currentThread(), thread.get());
	}

	void TearDown() {
		EXPECT_EQ(pipeline->poll(), nullptr);
		pipeline.reset();
		thread->quit();
		thread->wait();
		thread.reset();
	}
};

using ::testing::_;

TEST_F(PipelineTest, SyncIsInCurrentThread) {
	auto current = QThread::currentThread();
	EXPECT_CALL(*pipeline, onSyncMessage(_))
	    .Times(1)
	    .WillOnce([current](GstMessage *message) -> GstBusSyncReply {
		    EXPECT_STREQ(message->src->name, "utest");
		    EXPECT_EQ(QThread::currentThread(), current);

		    return GST_BUS_DROP;
	    });
	pipeline->postMessage();
}

TEST_F(PipelineTest, FollowsThreadAffinity) {
	auto current = QThread::currentThread();

	EXPECT_CALL(*pipeline, onSyncMessage(_))
	    .Times(1)
	    .WillOnce([current](GstMessage *message) -> GstBusSyncReply {
		    EXPECT_EQ(QThread::currentThread(), current);
		    return GST_BUS_PASS;
	    });

	std::atomic<bool> called{false};

	EXPECT_CALL(*pipeline, onMessage(_))
	    .Times(1)
	    .WillOnce([this, &called](GstMessage *message) -> GstBusSyncReply {
		    EXPECT_STREQ(message->src->name, "utest");
		    EXPECT_EQ(QThread::currentThread(), thread.get());
		    called.store(true);
		    called.notify_all();

		    return GST_BUS_PASS;
	    });

	pipeline->postMessage();

	auto future = std::make_unique<std::future<void>>(
	    std::async(std::launch::async, [&called] { called.wait(false); })
	);
	using namespace std::chrono_literals;
	if (future->wait_for(2s) == std::future_status::timeout) {
		ADD_FAILURE() << "Timeouted";
		future.release(); // intentional leak
	}
}

} // namespace yams
