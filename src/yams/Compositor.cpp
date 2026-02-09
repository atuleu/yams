#include "Compositor.hpp"

#include <chrono>

#include <cpptrace/exceptions.hpp>

#include <glib-object.h>
#include <gst/app/gstappsink.h>
#include <gst/gl/gstglbasememory.h>
#include <gst/gl/gstglcontext.h>
#include <gst/gl/gstgldisplay.h>
#include <gst/gl/gstglsyncmeta.h>
#include <gst/gstbin.h>
#include <gst/gstcaps.h>
#include <gst/gstelement.h>
#include <gst/gstmessage.h>
#include <gst/gstobject.h>
#include <gst/gstutils.h>

#include <yams/gstreamer/Thread.hpp>
#include <yams/utils/defer.hpp>

#include <slog++/slog++.hpp>

namespace yams {
GstElementPtr GstElementFactoryMake(const char *factory, const char *name) {
	auto res = gst_element_factory_make(factory, name);
	if (res == nullptr) {
		throw cpptrace::runtime_error{
		    "could not make element: gst_element_factory_make('" +
		    std::string(factory) + "','" + std::string{name} + "')"
		};
	}
	return GstElementPtr{res};
}

Compositor::Compositor(Options options, Args args)
    : Pipeline{"compositor0", args.Thread, args.Parent}
    , d_logger{slog::With(slog::String(
          "pipeline", (const char *)GST_OBJECT_NAME(d_pipeline.get())
      ))} {

	d_blacksrc = GstElementFactoryMake("videotestsrc", "blacksrc0");
	auto blackSourceCapsfilter =
	    GstElementFactoryMake("capsfilter", "blacksrcfilter0");
	d_compositor              = GstElementFactoryMake("glvideomixer", "vmix");
	auto compositorCapsfilter = GstElementFactoryMake("capsfilter", "vmixcaps");
	auto appsink              = GstElementFactoryMake("appsink", "sink0");

	auto blacksourceCaps = gst_caps_new_simple(
	    "video/x-raw",
	    "framerate",
	    "1/1",
	    "width",
	    16,
	    "height",
	    16,
	    nullptr
	);

	auto compositorCaps = gst_caps_new_simple(
	    "video/x-raw",
	    "framerate",
	    (std::to_string(options.FPS) + "/1").c_str(),
	    "width",
	    options.Size.width(),
	    "height",
	    options.Size.height(),
	    nullptr
	);
	defer {
		gst_caps_unref(blacksourceCaps);
		gst_caps_unref(compositorCaps);
	};

	g_object_set(
	    G_OBJECT(blackSourceCapsfilter.get()),
	    "caps",
	    blacksourceCaps,
	    nullptr
	);

	g_object_set(
	    G_OBJECT(compositorCapsfilter.get()),
	    "caps",
	    compositorCaps,
	    nullptr
	);
	using namespace std::chrono_literals;

	g_object_set(
	    G_OBJECT(d_compositor.get()),
	    "latency",
	    options.Latency.count(),
	    "background",
	    "black",
	    nullptr
	);

	g_object_set(
	    G_OBJECT(appsink.get()),
	    "emit-signals",
	    TRUE,
	    "sync",
	    TRUE,
	    nullptr
	);

	g_signal_connect(
	    appsink.get(),
	    "new-sample",
	    G_CALLBACK(&Compositor::onNewSampleCb),
	    this
	);

	g_object_set(G_OBJECT(d_blacksrc.get()), "pattern", "black", nullptr);

	gst_bin_add_many(
	    GST_BIN_CAST(d_pipeline.get()),
	    g_object_ref(d_blacksrc.get()),
	    g_object_ref(blackSourceCapsfilter.get()),
	    g_object_ref(d_compositor.get()),
	    g_object_ref(compositorCapsfilter.get()),
	    g_object_ref(appsink.get()),
	    nullptr
	);

	if (gst_element_link_many(
	        d_blacksrc.get(),
	        blackSourceCapsfilter.get(),
	        d_compositor.get(),
	        compositorCapsfilter.get(),
	        appsink.get(),
	        nullptr
	    ) == false) {
		throw cpptrace::runtime_error{"could not link pipeline elements"};
	}

	gst_bus_enable_sync_message_emission(d_bus.get());
	g_signal_connect(
	    d_bus.get(),
	    "sync-message",
	    G_CALLBACK(&Compositor::onSyncMessageCb),
	    this
	);
}

void Compositor::start(MediaPlayInfo media, int layer) {}

void Compositor::onMessage(GstMessage *msg) {
	switch (GST_MESSAGE_TYPE(msg)) {
	case GST_MESSAGE_EOS:
		d_logger.Info("EOS", slog::String("src", (const char *)msg->src->name));
		return;
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

		d_logger.Error(
		    "GStreamer Error",
		    slog::String("src", (const char *)msg->src->name),
		    slog::String("error", (const char *)err->message),
		    slog::String(
		        "debug",
		        debug == nullptr ? "none" : (const char *)debug
		    )
		);
		gst_element_set_state(d_pipeline.get(), GST_STATE_NULL);
		return;
	}
	default:
		return;
	}
}

gboolean
Compositor::onSyncMessageCb(GstBus *bus, GstMessage *msg, Compositor *self) {
	if (GST_MESSAGE_TYPE(msg) != GST_MESSAGE_NEED_CONTEXT) {
		return false;
	}
	const gchar *contextType;
	gst_message_parse_context_type(msg, &contextType);
	self->d_logger.Info(
	    "Gstreamer need context",
	    slog::String("type", contextType),
	    slog::String("source", msg->src->name)
	);
	if (g_strcmp0(contextType, GST_GL_DISPLAY_CONTEXT_TYPE) == 0) {
		GstContext *displayContext =
		    gst_context_new(GST_GL_DISPLAY_CONTEXT_TYPE, TRUE);
		gst_context_set_gl_display(displayContext, self->d_display);
		gst_element_set_context(GST_ELEMENT(msg->src), displayContext);
	} else if (g_strcmp0(contextType, "gst.gl.app_context") == 0) {
		self->d_logger.Debug(
		    "GstGL Objects",
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

GstFlowReturn Compositor::onNewSampleCb(GstElement *appsink, Compositor *self) {
	GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK_CAST(appsink));

	if (sample == nullptr) {
		return GST_FLOW_FLUSHING;
	}

	auto buffer = gst_buffer_ref(gst_sample_get_buffer(sample));
	gst_sample_unref(sample);

	if (self->d_gstContext == nullptr) {
		auto mem = gst_buffer_peek_memory(buffer, 0);
		self->d_gstContext.reset(
		    reinterpret_cast<GstGLBaseMemory *>(mem)->context
		);
		gst_object_ref(self->d_gstContext.get());
	}

	if (self->d_infos.has_value() == false) {
		self->d_infos = GstVideoInfo{};
		auto meta     = gst_buffer_get_video_meta(buffer);
		gst_video_info_set_format(
		    &(self->d_infos.value()),
		    meta->format,
		    meta->width,
		    meta->height
		);
		auto i = self->d_infos.value();
		self->d_logger.Info(
		    "got format",
		    slog::Group(
		        "infos",
		        slog::Int("width", i.width),
		        slog::Int("height", i.height)
		    )
		);
		emit self->videoInfos(self->d_infos.value());
	}

	auto syncMeta = gst_buffer_get_gl_sync_meta(buffer);
	if (syncMeta == nullptr) {
		buffer = gst_buffer_make_writable(buffer);
		syncMeta =
		    gst_buffer_add_gl_sync_meta(self->d_gstContext.get(), buffer);
	}
	// we mark that we are done with this texture from the Gstreamer context.
	gst_gl_sync_meta_set_sync_point(syncMeta, self->d_gstContext.get());

	// TODO: Use an object pool to avoid allocations?
	auto frame = g_new0(GstVideoFrame, 1);
	if (gst_video_frame_map(
	        frame,
	        &self->d_infos.value(),
	        buffer,
	        GstMapFlags(GST_MAP_READ | GST_MAP_GL)
	    ) == false) {
		self->d_logger.Error("failed to map video buffer");
		gst_buffer_unref(buffer);
		return GST_FLOW_ERROR;
	}
	gst_buffer_unref(buffer); // frame holds a ref from here

	emit self->newFrame((quintptr)frame);

	return GST_FLOW_OK;
}

}; // namespace yams
