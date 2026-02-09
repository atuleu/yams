#include "Pipeline.hpp"

#include <cpptrace/cpptrace.hpp>
#include <gst/gstbus.h>
#include <gst/gstelement.h>
#include <gst/gstmessage.h>
#include <gst/gstpipeline.h>

#include <yams/utils/defer.hpp>

namespace yams {
Pipeline::Pipeline(const char *name, QObject *parent)
    : QObject{parent} {

	d_pipeline = GstElementPtr{gst_pipeline_new(name)};
	if (d_pipeline == nullptr) {
		throw cpptrace::runtime_error{"could not set pipeline"};
	}

	d_bus = GstBusPtr{gst_pipeline_get_bus(GST_PIPELINE(d_pipeline.get()))};
	gst_bus_set_sync_handler(
	    d_bus.get(),
	    GstBusSyncHandler(&Pipeline::onBusSyncMessageCb),
	    this,
	    [](gpointer data) {}
	);
};

Pipeline::~Pipeline() {
	gst_element_set_state(d_pipeline.get(), GST_STATE_NULL);
}

GstBusSyncReply
Pipeline::onBusSyncMessageCb(GstBus *bus, GstMessage *msg, Pipeline *self) {

	if (self->onSyncMessage(msg) == GST_BUS_DROP) {
		// we handled the sync message and should not make async call
		gst_message_unref(msg);
		return GST_BUS_DROP;
	}

	// insert the method in the current QExecLoop. for async handling.
	QMetaObject::invokeMethod(
	    self,
	    &Pipeline::handleMessage,
	    Qt::QueuedConnection,
	    msg
	);

	return GST_BUS_DROP;
}

void Pipeline::handleMessage(GstMessage *msg) {
	this->onMessage(msg);
	gst_message_unref(msg);
}

} // namespace yams
