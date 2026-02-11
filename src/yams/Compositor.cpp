#include "Compositor.hpp"
#include "yams/MediaPipeline.hpp"

#include <chrono>

#include <gst/gstclock.h>
#include <gst/gstevent.h>
#include <gst/gstformat.h>
#include <gst/gstpad.h>
#include <gst/gstpipeline.h>
#include <qmetaobject.h>
#include <qnamespace.h>
#include <slog++/slog++.hpp>

#include <cpptrace/exceptions.hpp>

#include <glib-object.h>
#include <glib.h>
#include <gst/app/gstappsink.h>
#include <gst/gl/gstglbasememory.h>
#include <gst/gl/gstglcontext.h>
#include <gst/gl/gstgldisplay.h>
#include <gst/gl/gstglsyncmeta.h>
#include <gst/gstbin.h>
#include <gst/gstbus.h>
#include <gst/gstcaps.h>
#include <gst/gstcapsfeatures.h>
#include <gst/gstelement.h>
#include <gst/gstmessage.h>
#include <gst/gstobject.h>
#include <gst/gstutils.h>
#include <gst/gstvalue.h>

#include <yams/gstreamer/Factory.hpp>
#include <yams/gstreamer/Thread.hpp>
#include <yams/utils/defer.hpp>
#include <yams/utils/fractional.hpp>
#include <yams/utils/slogQt.hpp>

namespace yams {

struct Compositor::LayerData {
	Compositor    *self;
	MediaPipeline *pipeline;
	GstElementPtr  proxy;
	GstPadPtr      src, sink;
	GstEventPtr    segmentEvent;
};

void Compositor::stop() {
	gst_element_set_state(d_pipeline.get(), GST_STATE_NULL);
}

Compositor::Compositor(Options options, Args args)
    : Pipeline{"compositor0", args.Parent}
    , d_logger{slog::With(slog::String(
          "pipeline", (const char *)GST_OBJECT_NAME(d_pipeline.get())
      ))}
    , d_display{args.Display}
    , d_context{args.Context}
    , d_pool{FramePool::Create()}
    , d_size{options.Size} {

	if (options.Layers > 3) {
		throw cpptrace::invalid_argument{
		    "unsupported number of layers (" + std::to_string(options.Layers) +
		    ") max:3"
		};
	}

	d_clock =
	    GstClockPtr{gst_pipeline_get_clock(GST_PIPELINE(d_pipeline.get()))};

	auto clock = GstClockPtr{gst_system_clock_obtain()};
	gst_pipeline_use_clock(GST_PIPELINE(d_pipeline.get()), clock.get());

	d_blacksrc = GstElementFactoryMake("videotestsrc", "blacksrc0");
	auto blackSourceCapsfilter =
	    GstElementFactoryMake("capsfilter", "blacksrcfilter0");
	d_compositor              = GstElementFactoryMake("glvideomixer", "vmix");
	auto compositorCapsfilter = GstElementFactoryMake("capsfilter", "vmixcaps");
	auto appsink              = GstElementFactoryMake("appsink", "sink0");

	// clang-format off
	auto blacksourceCaps = gst_caps_new_simple(
	    "video/x-raw",
	    "framerate", GST_TYPE_FRACTION, 1, 1,
	    "width", G_TYPE_INT, 16,
	    "height", G_TYPE_INT, 16,
	    nullptr
	);

	auto [num,denum] = yams::build_fraction(options.FPS);

	d_logger.Info("output settings",
				  slog::String("framerate",std::to_string(num) + "/" + std::to_string(denum)),
				  slog::QSize("size",options.Size));

	auto compositorCaps = gst_caps_new_simple(
	    "video/x-raw",
	    "framerate",GST_TYPE_FRACTION, num,denum,
		"width", G_TYPE_INT, options.Size.width(),
	    "height", G_TYPE_INT, options.Size.height(),
	    nullptr
	);
	// clang-format on
	if (blacksourceCaps == nullptr) {
		throw cpptrace::runtime_error{"could not build black caps"};
	}
	if (compositorCaps == nullptr) {
		throw cpptrace::runtime_error{"could not build compositor caps"};
	}

	auto feature = gst_caps_features_new("memory:GLMemory", nullptr);
	gst_caps_set_features(compositorCaps, 0, feature);

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
	    0, // checker
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

	g_object_set(
	    G_OBJECT(d_blacksrc.get()),
	    "pattern",
	    2, // black
	    nullptr
	);

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

	buildLayers(options);
}

Compositor::~Compositor() {
	gst_element_set_state(d_pipeline.get(), GST_STATE_NULL);
}

void Compositor::start() {
	gst_element_set_state(d_pipeline.get(), GST_STATE_PLAYING);
}

void Compositor::play(const MediaPlayInfo &media, int layer) {
	auto runningTime = std::chrono::nanoseconds{
	    gst_clock_get_time(d_clock.get()) -
	    gst_element_get_base_time(d_pipeline.get())
	};
	if (QThread::currentThread() == this->thread()) {
		this->playUnsafe(media, layer, runningTime);
		return;
	}
	QMetaObject::invokeMethod(
	    this,
	    &Compositor::playUnsafe,
	    Qt::QueuedConnection,
	    media,
	    layer,
	    runningTime
	);
}

slog::Attribute slogGstSegment(const char *name, const GstSegment &segment) {
	return slog::Group(
	    name,
	    slog::Float("rate", segment.rate),
	    slog::Float("applied_rate", segment.applied_rate),
	    slog::String(
	        "format",
	        (const char *)gst_format_get_name(segment.format)
	    ),
	    slog::Int("base", segment.base),
	    slog::Int("offset", segment.offset),
	    slog::Int("start", segment.start),
	    slog::Int("stop", segment.stop),
	    slog::Int("time", segment.time),
	    slog::Int("position", segment.position),
	    slog::Int("duration", segment.duration)
	);
}

GstPadProbeReturn Compositor::onSinkEventProbe(
    GstPad *pad, GstPadProbeInfo *info, Compositor::LayerData *layer
) {
	auto event = GST_PAD_PROBE_INFO_EVENT(info);
	if (event == nullptr) {
		return GST_PAD_PROBE_OK;
	}
	auto logger = layer->self->d_logger;
	if (GST_EVENT_TYPE(event) == GST_EVENT_SEGMENT) {
		if (layer->segmentEvent == nullptr) {
			return GST_PAD_PROBE_OK;
		}
		const GstSegment *orig, *newSegment;
		gst_event_parse_segment(event, &orig);
		gst_event_parse_segment(layer->segmentEvent.get(), &newSegment);
		layer->self->d_logger.Info(
		    "replacing event",
		    slogGstSegment("orig", *orig),
		    slogGstSegment("new", *newSegment)
		);

		// gst_pad_send_event(pad, layer->segmentEvent.release());
		return GST_PAD_PROBE_OK;
	}
	if (GST_EVENT_TYPE(event) != GST_EVENT_EOS) {
		return GST_PAD_PROBE_OK;
	}
	gst_pad_remove_probe(pad, GST_PAD_PROBE_INFO_ID(info));
	QMetaObject::invokeMethod(
	    layer->self,
	    &Compositor::removeMedia,
	    Qt::QueuedConnection,
	    layer
	);
	return GST_PAD_PROBE_OK;
}

void Compositor::playUnsafe(
    const MediaPlayInfo &media, int layerIndex, std::chrono::nanoseconds from
) {
	if (layerIndex != 0) {
		d_logger.Warn("only single layer supported");
		return;
	}

	auto layer = d_layers[layerIndex].get();

	g_object_set(
	    layer->proxy.get(),
	    "proxysink",
	    layer->pipeline->proxySink(),
	    nullptr
	);

	if (gst_element_link_many(
	        layer->proxy.get(),
	        d_compositor.get(),
	        nullptr
	    ) == false) {

		d_logger.Error("could not link proxysink to compositor");
		return;
	}

	layer->src.reset(gst_element_get_static_pad(layer->proxy.get(), "src"));
	if (layer->src == nullptr) {
		d_logger.Error("could not find the src pad?");
		return;
	}

	layer->sink.reset(gst_pad_get_peer(layer->src.get()));
	if (layer->sink == nullptr) {
		d_logger.Error("could not find sink pad on compositor");
		return;
	}
	g_object_set(
	    layer->sink.get(),
	    "width",
	    d_size.width(),
	    "height",
	    d_size.height(),
	    "sizing-policy",
	    1,
	    nullptr
	);
	GstSegment segment;
	gst_segment_init(&segment, GST_FORMAT_TIME);
	segment.offset = from.count();
	segment.start  = 0;
	segment.stop   = GST_CLOCK_TIME_NONE;
	segment.rate   = 1.0;
	layer->segmentEvent.reset(gst_event_new_segment(&segment));

	gst_pad_add_probe(
	    layer->sink.get(),
	    GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM,
	    (GstPadProbeCallback)&Compositor::onSinkEventProbe,
	    layer,
	    nullptr
	);

	d_logger.Info(
	    "starting",
	    slog::String("media", media.Location.toStdString()),
	    slog::Duration("offset", from)
	);
	layer->pipeline->play(media, from);
}

void Compositor::removeMedia(LayerData *layer) {
	if (gst_pad_unlink(layer->src.get(), layer->sink.get()) == false) {
		d_logger.Error(
		    "could not unlink pads",
		    slog::String(
		        "source",
		        (const char *)GST_OBJECT_NAME(layer->src.get())
		    ),
		    slog::String(
		        "sink",
		        (const char *)GST_OBJECT_NAME(layer->sink.get())
		    )
		);
		return;
	}
	layer->src.reset();
	gst_element_release_request_pad(d_compositor.get(), layer->sink.get());
	layer->sink.reset();
}

void Compositor::onMessage(GstMessage *msg) noexcept {
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

GstBusSyncReply Compositor::onSyncMessage(GstMessage *msg) noexcept {
	if (GST_MESSAGE_TYPE(msg) != GST_MESSAGE_NEED_CONTEXT) {
		return GST_BUS_PASS;
	}
	const gchar *contextType;
	gst_message_parse_context_type(msg, &contextType);
	d_logger.Debug(
	    "Gstreamer need context",
	    slog::String("type", contextType),
	    slog::String("source", (const char *)msg->src->name)
	);
	if (g_strcmp0(contextType, GST_GL_DISPLAY_CONTEXT_TYPE) == 0) {
		GstContext *displayContext =
		    gst_context_new(GST_GL_DISPLAY_CONTEXT_TYPE, TRUE);
		gst_context_set_gl_display(displayContext, d_display);
		gst_element_set_context(GST_ELEMENT(msg->src), displayContext);
		slog::Debug(
		    "handled",
		    slog::String("type", contextType),
		    slog::String("source", (const char *)msg->src->name),
		    slog::Pointer("display", d_display)
		);
		return GST_BUS_DROP;
	}
	if (g_strcmp0(contextType, "gst.gl.app_context") == 0) {
		d_logger.Debug(
		    "GstGL Objects",
		    slog::Pointer("display", d_display),
		    slog::Pointer("context", d_context)
		);
		GstContext   *appContext = gst_context_new("gst.gl.app_context", TRUE);
		GstStructure *s          = gst_context_writable_structure(appContext);
		gst_structure_set(
		    s,
		    "context",
		    GST_TYPE_GL_CONTEXT,
		    d_context,
		    nullptr
		);
		gst_element_set_context(GST_ELEMENT(msg->src), appContext);
		slog::Debug(
		    "handled",
		    slog::String("type", contextType),
		    slog::String("source", (const char *)msg->src->name)
		);
		return GST_BUS_DROP;
	}

	return GST_BUS_PASS;
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
		emit self->outputSizeChanged(
		    {self->d_infos.value().width, self->d_infos.value().height}
		);
	}

	auto syncMeta = gst_buffer_get_gl_sync_meta(buffer);
	if (syncMeta == nullptr) {
		buffer = gst_buffer_make_writable(buffer);
		syncMeta =
		    gst_buffer_add_gl_sync_meta(self->d_gstContext.get(), buffer);
	}
	// we mark that we are done with this texture from the Gstreamer
	// context.
	gst_gl_sync_meta_set_sync_point(syncMeta, self->d_gstContext.get());

	auto frame =
	    Frame::Ptr{self->d_pool->Get([](Frame *frame) { frame->unmap(); })};
	auto allocated = self->d_pool->PoolSize();
	if (allocated < 10) {
		self->d_logger.DDebug(
		    "GstVideoFrame allocation",
		    slog::Int("allocated", allocated)
		);
	} else if (allocated < 20) {
		self->d_logger.Warn(
		    "GstVideoFrame allocation",
		    slog::Int("allocated", allocated)
		);
	} else {
		self->d_logger.Error(
		    "Large GstVideoFrame allocation, make sure output frame are "
		    "disposed.",
		    slog::Int("allocated", allocated)
		);
	}

	if (frame->map(
	        &self->d_infos.value(),
	        buffer,
	        GstMapFlags(GST_MAP_READ | GST_MAP_GL)
	    ) == false) {
		self->d_logger.Error("failed to map video buffer");
		gst_buffer_unref(buffer);
		return GST_FLOW_ERROR;
	}
	gst_buffer_unref(buffer); // frame holds a ref from here

	emit self->newFrame(frame);

	return GST_FLOW_OK;
}

void Compositor::buildLayers(const Options &opts) {
	for (size_t i = 0; i < opts.Layers; ++i) {
		auto proxySrc = GstElementFactoryMake(
		    "proxysrc",
		    ("videosrc" + std::to_string(i)).c_str()
		);

		auto pipeline = new MediaPipeline{
		    {.ID = i, .Size = opts.Size, .FPS = opts.FPS},
		    this
		};

		gst_bin_add(GST_BIN(d_pipeline.get()), g_object_ref(proxySrc.get()));

		d_layers.emplace_back(new LayerData{
		    .self     = this,
		    .pipeline = pipeline,
		    .proxy    = std::move(proxySrc)
		});
	}
}

} // namespace yams
