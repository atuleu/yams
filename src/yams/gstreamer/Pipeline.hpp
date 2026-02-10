#pragma once

#include <gst/gstbus.h>
#include <qtmetamacros.h>
#include <yams/gstreamer/Memory.hpp>
#include <yams/gstreamer/Thread.hpp>

namespace yams {
class Pipeline : public QObject {
	Q_OBJECT
public:
	Pipeline(const char *name, QObject *parent);
	virtual ~Pipeline();
	Pipeline(const Pipeline &)            = delete;
	Pipeline(Pipeline &&)                 = delete;
	Pipeline &operator=(const Pipeline &) = delete;
	Pipeline &operator=(Pipeline &&)      = delete;

protected:
	virtual void onMessage(GstMessage *msg) noexcept {
		// we need to be defined to receive message when the destructor are
		// called.
	}

	virtual GstBusSyncReply onSyncMessage(GstMessage *msg) noexcept {
		return GST_BUS_PASS;
	};

private slots:
	void handleMessage(GstMessage *msg);

protected:
	static GstBusSyncReply
	onBusSyncMessageCb(GstBus *bus, GstMessage *message, Pipeline *pipeline);

	GstElementPtr d_pipeline;
	GstBusPtr     d_bus;
};
} // namespace yams
