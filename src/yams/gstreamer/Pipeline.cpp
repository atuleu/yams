#include "Pipeline.hpp"

#include <cpptrace/cpptrace.hpp>
#include <gst/gstelement.h>
#include <gst/gstpipeline.h>

namespace yams {
Pipeline::Pipeline(const char *name, GstThread *thread, QObject *parent)
    : QObject{parent}
    , d_thread{thread} {

	if (thread == nullptr) {
		throw cpptrace::logic_error{"thread should be defined"};
	}

	if (parent == thread) {
		throw cpptrace::logic_error{"parent object should not be its thread"};
	}
	moveToThread(thread);

	d_pipeline = GstElementPtr{gst_pipeline_new(name)};
	if (d_pipeline == nullptr) {
		throw cpptrace::runtime_error{"could not set pipeline"};
	}

	d_bus = GstBusPtr{gst_pipeline_get_bus(GST_PIPELINE(d_pipeline.get()))};
	d_thread->watchBus(d_bus.get(), GstBusFunc(&Pipeline::onMessageCb), this);
};

Pipeline::~Pipeline() {
	d_thread->unwatchBus(d_bus.get());
	gst_element_set_state(d_pipeline.get(), GST_STATE_NULL);
}

gboolean Pipeline::onMessageCb(GstBus *bus, GstMessage *msg, Pipeline *self) {
	self->onMessage(msg);
	return true;
}

} // namespace yams
