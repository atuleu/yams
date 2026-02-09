#include "VideoThread.hpp"

#include <glib-object.h>
#include <glib.h>
#include <gst/gl/gstgl_fwd.h>
#include <gst/gl/gstglapi.h>
#include <gst/gl/gstglcontext.h>
#include <gst/gl/gstgldisplay.h>
#include <gst/gl/gstglmemory.h>
#include <gst/gl/gstglsyncmeta.h>
#include <gst/gstbuffer.h>
#include <gst/gstbus.h>
#include <gst/gstcontext.h>
#include <gst/gstelement.h>
#include <gst/gstmemory.h>
#include <gst/gstmessage.h>
#include <gst/gstpad.h>
#include <gst/gstsample.h>
#include <gst/video/video-info.h>
#include <qopenglcontext.h>
#include <qtpreprocessorsupport.h>
#include <slog++/slog++.hpp>

#include <yams/gstreamer/memory.hpp>
#include <yams/utils/defer.hpp>
#include <yams/utils/slogQt.hpp>

#include <QGuiApplication>

namespace yams {

VideoThread::VideoThread(
    GstGLDisplay *display, GstGLContext *context, QObject *parent
)
    : yams::GstThreadLegacy{parent}
    , d_display{display}
    , d_context{context} {
	d_pipeline    = GstElementPtr{gst_pipeline_new("redraw")};
	auto source   = gst_element_factory_make("videotestsrc", "source0");
	auto convert  = gst_element_factory_make("videoconvert", "convert0");
	auto glUpload = gst_element_factory_make("glupload", "glupload0");
	auto sink     = gst_element_factory_make("appsink", "sink0");
	if (!d_pipeline || !source || !convert || !glUpload || !sink) {
		slog::Fatal("could not create element");
	}

	g_object_set(G_OBJECT(sink), "emit-signals", TRUE, "sync", TRUE, nullptr);

	g_signal_connect(
	    sink,
	    "new-sample",
	    G_CALLBACK(VideoThread::onNewSampleCb),
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

GstFlowReturn
VideoThread::onNewSampleCb(GstElement *appsink, VideoThread *self) {
	return self->onNewSample(appsink);
}

GstFlowReturn VideoThread::onNewSample(GstElement *appsink) {
	GstSample *sample{nullptr};
	g_signal_emit_by_name(appsink, "pull-sample", &sample, nullptr);

	if (sample == nullptr) {
		return GST_FLOW_FLUSHING;
	}

	auto buffer = gst_buffer_ref(gst_sample_get_buffer(sample));
	gst_sample_unref(sample);

	if (d_gstContext == nullptr) {
		auto mem = gst_buffer_peek_memory(buffer, 0);
		d_gstContext.reset(reinterpret_cast<GstGLBaseMemory *>(mem)->context);
		gst_object_ref(d_gstContext.get());
	}

	if (d_infos.has_value() == false) {
		d_infos   = GstVideoInfo{};
		auto meta = gst_buffer_get_video_meta(buffer);
		gst_video_info_set_format(
		    &d_infos.value(),
		    meta->format,
		    meta->width,
		    meta->height
		);
		auto i = d_infos.value();
		slog::Info(
		    "format",
		    slog::Group(
		        "infos",
		        slog::Int("width", i.width),
		        slog::Int("height", i.height)
		    )
		);
	}

	auto syncMeta = gst_buffer_get_gl_sync_meta(buffer);
	if (syncMeta == nullptr) {
		buffer   = gst_buffer_make_writable(buffer);
		syncMeta = gst_buffer_add_gl_sync_meta(d_gstContext.get(), buffer);
	}
	gst_gl_sync_meta_set_sync_point(syncMeta, d_gstContext.get());

	auto frame = g_new0(GstVideoFrame, 1);
	if (gst_video_frame_map(
	        frame,
	        &d_infos.value(),
	        buffer,
	        GstMapFlags(GST_MAP_READ | GST_MAP_GL)
	    ) == false) {
		slog::Warn("failed to map video buffer");
		gst_buffer_unref(buffer);
		return GST_FLOW_ERROR;
	}
	gst_buffer_unref(buffer); // frame holds a ref from here

	emit newFrame(
	    (quintptr)frame,
	    QSize{d_infos.value().width, d_infos.value().height}
	);

	return GST_FLOW_OK;
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
