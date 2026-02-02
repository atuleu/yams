#include "VideoThread.hpp"

#include <gst/gstelement.h>
#include <slog++/slog++.hpp>

#include "defer.hpp"

namespace yams {

VideoThread::VideoThread(QObject *parent)
    : yams::GstThread{parent} {
	d_pipeline       = details::GstElementPtr{gst_pipeline_new("redraw")};
	auto source      = gst_element_factory_make("videotestsrc", "source0");
	auto convert     = gst_element_factory_make("videoconvert", "convert0");
	auto capsConvert = gst_element_factory_make("capsfilter", "caps0");
	auto sink        = gst_element_factory_make("fakesink", "sink0");
	if (!d_pipeline || !source || !convert || !capsConvert || !sink) {
		slog::Fatal("could not create element");
	}
	auto caps =
	    gst_caps_from_string("video/x-raw,format=BGR,width=1920,height=1080");
	g_object_set(G_OBJECT(capsConvert), "caps", caps, nullptr);

	g_object_set(
	    G_OBJECT(sink),
	    "signal-handoffs",
	    TRUE,
	    "sync",
	    TRUE,
	    nullptr
	);

	g_signal_connect(
	    sink,
	    "handoff",
	    G_CALLBACK(VideoThread::onNewFrameCb),
	    this
	);

	gst_bin_add_many(
	    GST_BIN(d_pipeline.get()),
	    source,
	    convert,
	    capsConvert,
	    sink,
	    nullptr
	);
	if (gst_element_link_many(source, convert, capsConvert, sink, nullptr) ==
	    false) {
		slog::Fatal("could not link elements");
	}

	auto bus =
	    details::GstBusPtr{gst_pipeline_get_bus(GST_PIPELINE(d_pipeline.get()))
	    };

	gst_bus_add_watch(bus.get(), &VideoThread::onNewMessageCb, this);
}

void VideoThread::startTask() {
	gst_element_set_state(d_pipeline.get(), GST_STATE_PLAYING);
}

void VideoThread::doneTask() {
	gst_element_set_state(d_pipeline.get(), GST_STATE_NULL);
}

void VideoThread::onNewFrame(GstBuffer *buffer) {
	gst_buffer_ref(buffer);
	emit newFrame(buffer);
}

void VideoThread::onNewFrameCb(
    GstElement *fakesink, GstBuffer *buffer, GstPad *pad, gpointer userdata
) {
	auto self = reinterpret_cast<VideoThread *>(userdata);
	if (!buffer || !self) {
		return;
	};
	self->onNewFrame(buffer);
}

gboolean
VideoThread::onNewMessageCb(GstBus *bus, GstMessage *msg, gpointer userdata) {
	auto self = reinterpret_cast<VideoThread *>(userdata);
	switch (GST_MESSAGE_TYPE(msg)) {
	case GST_MESSAGE_EOS:
		slog::Info("EOS");
		self->stop();
		break;

	case GST_MESSAGE_ERROR: {
		gchar  *debug{nullptr};
		GError *err{nullptr};
		gst_message_parse_error(msg, &err, &debug);
		defer {
			g_error_free(err);
			if (debug) {
				g_free(debug);
			}
		};
		slog::Error(
		    "bus error",
		    slog::String("source", msg->src->name),
		    slog::String("error", err->message),
		    slog::String("debug", debug != nullptr ? debug : "none")
		);
		self->stop();
		break;
	}
	default:
		break;
	}
	return TRUE;
}

void VideoThread::stopPipeline() {
	gst_element_send_event(d_pipeline.get(), gst_event_new_eos());
}
} // namespace yams
