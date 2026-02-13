#include "MediaPipeline.hpp"
#include "yams/MediaPlayInfo.hpp"
#include "yams/utils/defer.hpp"

#include <chrono>
#include <glib-object.h>
#include <gst/gstcaps.h>
#include <gst/gstclock.h>
#include <gst/gstelement.h>
#include <gst/gstmessage.h>
#include <gst/gstpipeline.h>
#include <gst/gstsystemclock.h>
#include <gst/gstutils.h>

#include <cpptrace/exceptions.hpp>

#include <optional>
#include <string>
#include <yams/gstreamer/Factory.hpp>
#include <yams/utils/fractional.hpp>

namespace yams {

MediaPipeline::MediaPipeline(Args args, Compositor *parent)
    : Pipeline{("media" + std::to_string(args.LayerID) + "_"+std::to_string(args.SinkID)).c_str(), (QObject *)parent}
    , d_logger{slog::With(slog::String(
          "pipeline", (const char *)GST_OBJECT_NAME(d_pipeline.get())
      ))} {

	auto clock = GstClockPtr{gst_system_clock_obtain()};
	gst_pipeline_use_clock(GST_PIPELINE(d_pipeline.get()), clock.get());

	auto [num, denum] = yams::build_fraction(args.FPS);
	d_framerateNum    = num;
	d_framerateDenum  = denum;

	// clang-format off
	auto testCaps = gst_caps_new_simple(
	    "video/x-raw", //
	    "framerate", GST_TYPE_FRACTION, num, denum,
		"width", G_TYPE_INT, args.Size.width(),
		"height", G_TYPE_INT, args.Size.height(),
	    nullptr
	);
	// clang-format on
	if (testCaps == nullptr) {
		throw cpptrace::logic_error{"invalid videotestsrc caps"};
	}
	defer {
		gst_caps_unref(testCaps);
	};

	auto decodeCaps = gst_caps_new_simple(
	    "video/x-raw",
	    "format",
	    G_TYPE_STRING,
	    "RGBA",
	    nullptr
	);
	if (decodeCaps == nullptr) {
		throw cpptrace::logic_error{"invalid decode caps"};
	}

	d_fileSource = GstElementFactoryMakeFull("filesrc", "name", "file0");

	d_testSource = GstElementFactoryMakeFull("videotestsrc", "name", "test0");
	d_testCapsfilter = GstElementFactoryMakeFull(
	    "capsfilter",
	    "name",
	    "testCaps0",
	    "caps",
	    testCaps
	);
	d_timeOverlay =
	    GstElementFactoryMakeFull("timeoverlay", "name", "timeoverlay0");
	d_proxySink = GstElementFactoryMakeFull("proxysink", "name", "sink0");
	d_decodeBin = GstElementFactoryMakeFull("decodebin", "name", "decode0");
	d_decodeCapsfilter = GstElementFactoryMakeFull(
	    "capsfilter",
	    "name",
	    "decodeCaps0",
	    "caps",
	    decodeCaps
	);

	d_queue = GstElementFactoryMakeFull("queue", "name", "queue0");

	gst_bin_add_many(
	    GST_BIN(d_pipeline.get()),
	    g_object_ref(d_queue.get()),
	    g_object_ref(d_proxySink.get()),
	    nullptr
	);
	if (gst_element_link_many(d_queue.get(), d_proxySink.get(), nullptr) ==
	    false) {
		throw cpptrace::runtime_error("could not link final element");
	}
}

MediaPipeline::~MediaPipeline() {
	gst_element_set_state(d_pipeline.get(), GST_STATE_NULL);
}

GstElement *MediaPipeline::proxySink() {
	return d_proxySink.get();
}

void MediaPipeline::play(const MediaPlayInfo &infos) {
	if (d_playing == true) {
		d_logger.Error("already playing");
		return;
	}
	d_currentMedia = infos.MediaType;
	switch (infos.MediaType) {
	case MediaPlayInfo::Type::IMAGE:
	case MediaPlayInfo::Type::VIDEO:
		playFile(infos);
		break;
	case MediaPlayInfo::Type::TEST:
		playTest(infos);
		break;
	}
}

void MediaPipeline::onMessage(GstMessage *msg) noexcept {
	switch (GST_MESSAGE_TYPE(msg)) {
	case GST_MESSAGE_ERROR: {
		gchar  *debug{nullptr};
		GError *err;
		gst_message_parse_error(msg, &err, &debug);
		defer {
			g_error_free(err);
			if (debug) {
				g_free(debug);
			}
		};
		d_logger.Error(
		    "Gstreamer error",
		    slog::String("src", (const char *)msg->src->name),
		    slog::String("error", (const char *)err->message),
		    slog::String(
		        "debug",
		        debug == nullptr ? "none" : (const char *)debug
		    )
		);
		onError();
		break;
	}
	case GST_MESSAGE_EOS:
		d_logger.Info("EOS", slog::String("src", (const char *)msg->src->name));
		onEOS();
		break;
	case GST_MESSAGE_STATE_CHANGED: {
		if (GST_OBJECT(d_pipeline.get()) != msg->src) {
			break;
		}
		GstState oldState, newState, pending;

		gst_message_parse_state_changed(msg, &oldState, &newState, &pending);
		if (newState != GST_STATE_NULL) {
			break;
		}
		reset();
	}
	}
}

void MediaPipeline::stop() {
	if (d_playing == true) {
		forceDownstreamEOS();
	}
	onEOS();
}

void MediaPipeline::onEOS() {
	emit EOS();
	gst_element_set_state(d_pipeline.get(), GST_STATE_NULL);
}

void MediaPipeline::forceDownstreamEOS() {
	GstEvent *eosEvent = gst_event_new_eos();
	gst_element_send_event(d_proxySink.get(), eosEvent);
}

void MediaPipeline::onError() {
	// send EOS event on proxySink
	forceDownstreamEOS();
	emit Error();
	onEOS();
}

void MediaPipeline::reset() {
	if (d_currentMedia.has_value()) {
		switch (d_currentMedia.value()) {
		case MediaPlayInfo::Type::IMAGE:
		case MediaPlayInfo::Type::VIDEO:
			gst_element_unlink_many(
			    d_fileSource.get(),
			    d_decodeBin.get(),
			    d_decodeCapsfilter.get(),
			    d_queue.get(),
			    nullptr
			);
			gst_bin_remove_many(
			    GST_BIN(d_pipeline.get()),
			    d_fileSource.get(),
			    d_decodeBin.get(),
			    d_decodeCapsfilter.get(),
			    nullptr
			);
			break;
		case MediaPlayInfo::Type::TEST:
			gst_element_unlink_many(
			    d_testSource.get(),
			    d_testCapsfilter.get(),
			    d_timeOverlay.get(),
			    d_queue.get(),
			    nullptr
			);
			gst_bin_remove_many(
			    GST_BIN(d_pipeline.get()),
			    d_testSource.get(),
			    d_testCapsfilter.get(),
			    d_timeOverlay.get(),
			    nullptr
			);
			break;
		}
	}

	d_playing      = false;
	d_currentMedia = std::nullopt;
}

void MediaPipeline::playFile(const MediaPlayInfo &infos) {
	g_object_set(
	    d_fileSource.get(),
	    "location",
	    infos.Location.toStdString().c_str(),
	    nullptr
	);

	gst_bin_add_many(
	    GST_BIN(d_pipeline.get()),
	    g_object_ref(d_fileSource.get()),
	    g_object_ref(d_decodeBin.get()),
	    g_object_ref(d_decodeCapsfilter.get()),
	    nullptr
	);
	if (gst_element_link_many(
	        d_fileSource.get(),
	        d_decodeBin.get(),
	        d_decodeCapsfilter.get(),
	        d_queue.get(),
	        nullptr
	    ) == false) {
		d_logger.Error("could not link file pipeline");
		gst_bin_remove_many(
		    GST_BIN(d_pipeline.get()),
		    d_fileSource.get(),
		    d_decodeBin.get(),
		    d_decodeCapsfilter.get(),
		    nullptr
		);
		return;
	}
	gst_element_set_state(d_pipeline.get(), GST_STATE_PLAYING);
	d_playing      = true;
	d_currentMedia = infos.MediaType;
}

void MediaPipeline::playTest(const MediaPlayInfo &infos) {
	static std::map<std::string, int> patternByName = {
	    {"smpte", 0},              // SMPTE 100%% color bars
	    {"snow", 1},               // Random (television snow)
	    {"black", 2},              // 100%% Black
	    {"white", 3},              // 100%% White
	    {"red", 4},                // Red
	    {"green", 5},              // Green
	    {"blue", 6},               // Blue
	    {"checkers-1", 7},         // Checkers 1px
	    {"checkers-2", 8},         // Checkers 2px
	    {"checkers-4", 9},         // Checkers 4px
	    {"checkers-8", 10},        // Checkers 8px
	    {"circular", 11},          // Circular
	    {"blink", 12},             // Blink
	    {"smpte75", 13},           // SMPTE 75%% color bars
	    {"zone-plate", 14},        // Zone plate
	    {"gamut", 15},             // Gamut checkers
	    {"chroma-zone-plate", 16}, // Chroma zone plate
	    {"solid-color", 17},       // Solid color
	    {"ball", 18},              // Moving ball
	    {"smpte100", 19},          // SMPTE 100%% color bars
	    {"bar", 20},               // Bar
	    {"pinwheel", 21},          // Pinwheel
	    {"spokes", 22},            // Spokes
	    {"gradient", 23},          // Gradient
	    {"colors", 24},            // Colors
	    {"smpte-rp-219", 25},      // SMPTE test pattern, RP 219 conformant
	};
	auto patternName = infos.Location.toStdString();
	if (patternByName.count(patternName) == 0) {
		d_logger.Error(
		    "unknown pattern name",
		    slog::String("pattern", infos.Location.toStdString())
		);
		return;
	}
	using namespace std::chrono_literals;
	uint64_t buffers = infos.Duration.count() * d_framerateNum /
	                   d_framerateDenum / std::chrono::nanoseconds{1s}.count();
	d_logger.Info(
	    "test media duration",
	    slog::Duration("duration", infos.Duration),
	    slog::Int("buffers", buffers),
	    slog::String(
	        "FPS",
	        std::to_string(d_framerateNum) + "/" +
	            std::to_string(d_framerateDenum)
	    )
	);

	g_object_set(
	    d_testSource.get(),
	    "pattern",
	    patternByName.at(patternName),
	    "num-buffers",
	    buffers,
	    nullptr
	);

	auto bin = GST_BIN(d_pipeline.get());
	gst_bin_add_many(
	    bin,
	    g_object_ref(d_testSource.get()),
	    g_object_ref(d_testCapsfilter.get()),
	    g_object_ref(d_timeOverlay.get()),
	    nullptr
	);
	if (gst_element_link_many(
	        d_testSource.get(),
	        d_testCapsfilter.get(),
	        d_timeOverlay.get(),
	        d_queue.get(),
	        nullptr
	    ) == false) {
		d_logger.Error("could not link test pipeline");
		gst_bin_remove_many(
		    bin,
		    d_testSource.get(),
		    d_testCapsfilter.get(),
		    d_timeOverlay.get(),
		    nullptr
		);
		return;
	}

	gst_element_set_state(d_pipeline.get(), GST_STATE_PLAYING);
	d_playing      = true;
	d_currentMedia = MediaPlayInfo::Type::TEST;
}

} // namespace yams
