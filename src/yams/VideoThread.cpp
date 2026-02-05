#include "VideoThread.hpp"

#include <glib.h>
#include <gst/gl/gstgl_fwd.h>
#include <gst/gl/gstglapi.h>
#include <gst/gl/gstglcontext.h>
#include <gst/gl/gstgldisplay.h>
#include <gst/gstbus.h>
#include <gst/gstcontext.h>
#include <gst/gstelement.h>
#include <gst/gstmessage.h>
#include <qopenglcontext.h>
#include <qtpreprocessorsupport.h>
#include <slog++/slog++.hpp>

#include "defer.hpp"
#include "yams/gstreamer.hpp"

#include <QGuiApplication>

namespace yams {

VideoThread::VideoThread(
    GstGLDisplay *display, GstGLContext *context, QObject *parent
)
    : yams::GstThread{parent}
    , d_display{display}
    , d_context{context} {
	d_pipeline    = GstElementPtr{gst_pipeline_new("redraw")};
	auto source   = gst_element_factory_make("videotestsrc", "source0");
	auto convert  = gst_element_factory_make("videoconvert", "convert0");
	auto glUpload = gst_element_factory_make("glupload", "glupload0");
	auto sink     = gst_element_factory_make("fakesink", "sink0");
	if (!d_pipeline || !source || !convert || !glUpload || !sink) {
		slog::Fatal("could not create element");
	}

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
	    glUpload,
	    sink,
	    nullptr
	);
	if (gst_element_link_many(source, convert, glUpload, sink, nullptr) ==
	    false) {
		slog::Fatal("could not link elements");
	}

	auto bus = GstBusPtr{gst_pipeline_get_bus(GST_PIPELINE(d_pipeline.get()))};

	gst_bus_add_watch(bus.get(), &VideoThread::onNewMessageCb, this);
	gst_bus_enable_sync_message_emission(bus.get());
	g_signal_connect(
	    bus.get(),
	    "sync-message",
	    G_CALLBACK(&VideoThread::onSyncMessageCb),
	    this
	);
}

void VideoThread::startTask() {
	gst_element_set_state(d_pipeline.get(), GST_STATE_PAUSED);
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
    GstElement *fakesink, GstBuffer *buffer, GstPad *pad, VideoThread *self
) {
	Q_UNUSED(pad);
	Q_UNUSED(fakesink);
	// we ref the buffer.

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
		    slog::String("source", (const char *)msg->src->name),
		    slog::String("error", (const char *)err->message),
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

gboolean
VideoThread::onSyncMessageCb(GstBus *bus, GstMessage *msg, VideoThread *self) {
	if (GST_MESSAGE_TYPE(msg) != GST_MESSAGE_NEED_CONTEXT) {
		return false;
	}
	const gchar *contextType;
	gst_message_parse_context_type(msg, &contextType);
	slog::Info("Gstreamer need context", slog::String("type", contextType));
	if (g_strcmp0(contextType, GST_GL_DISPLAY_CONTEXT_TYPE) == 0) {
		GstContext *displayContext =
		    gst_context_new(GST_GL_DISPLAY_CONTEXT_TYPE, TRUE);
		gst_context_set_gl_display(displayContext, self->d_display);
		gst_element_set_context(GST_ELEMENT(msg->src), displayContext);
	} else if (g_strcmp0(contextType, "gst.gl.app_context") == 0) {
		slog::Info(
		    "GL things",
		    slog::Pointer("display", self->d_display),
		    slog::Pointer("context", self->d_context)
		);
		GstContext   *appContext = gst_context_new("gst.gl.app_context", TRUE);
		GstStructure *s          = gst_context_writable_structure(appContext);
		gst_structure_set(
		    s,
		    "context",
		    GST_TYPE_GL_CONTEXT,
		    self->d_context,
		    nullptr
		);
		gst_element_set_context(GST_ELEMENT(msg->src), appContext);
	}

	return false;
}

void VideoThread::stopPipeline() {
	gst_element_send_event(d_pipeline.get(), gst_event_new_eos());
}
} // namespace yams
