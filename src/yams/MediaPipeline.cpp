#include "MediaPipeline.hpp"
#include "yams/MediaPlayInfo.hpp"
#include "yams/utils/defer.hpp"

#include <glib-object.h>
#include <gst/gstclock.h>
#include <gst/gstelement.h>
#include <gst/gstmessage.h>
#include <gst/gstpipeline.h>
#include <gst/gstsystemclock.h>
#include <gst/gstutils.h>

#include <cpptrace/exceptions.hpp>

#include <optional>
#include <yams/gstreamer/Factory.hpp>
#include <yams/utils/fractional.hpp>

namespace yams {

MediaPipeline::MediaPipeline(Args args, Compositor *parent)
    : Pipeline{("media" + std::to_string(args.ID)).c_str(), (QObject *)parent}
    , d_logger{slog::With(slog::String(
          "pipeline", (const char *)GST_OBJECT_NAME(d_pipeline.get())
      ))} {

	auto clock = GstClockPtr{gst_system_clock_obtain()};
	gst_pipeline_use_clock(GST_PIPELINE(d_pipeline.get()), clock.get());

	d_fileSource = GstElementFactoryMake("filesrc", "file0");

	d_testSource     = GstElementFactoryMake("videotestsrc", "test0");
	d_testCapsfilter = GstElementFactoryMake("capsfilter", "testCaps0");
	d_timeOverlay    = GstElementFactoryMake("timeoverlay", "timeoverlay0");

	d_proxySink        = GstElementFactoryMake("proxysink", "sink0");
	d_decodeBin        = GstElementFactoryMake("decodebin", "decode0");
	d_decodeCapsfilter = GstElementFactoryMake("capsfilter", "decodeCaps0");

	auto [num, denum] = yams::build_fraction(args.FPS);

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

	g_object_set(G_OBJECT(d_testCapsfilter.get()), "caps", testCaps, nullptr);

	gst_bin_add_many(
	    GST_BIN(d_pipeline.get()),
	    g_object_ref(d_proxySink.get()),
	    nullptr
	);
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

void MediaPipeline::onEOS() {
	gst_element_set_state(d_pipeline.get(), GST_STATE_NULL);
}

void MediaPipeline::onError() {
	// send EOS event on proxySink
	GstEvent *eosEvent = gst_event_new_eos();
	gst_element_send_event(d_proxySink.get(), eosEvent);
	gst_element_set_state(d_pipeline.get(), GST_STATE_NULL);
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
			    d_proxySink.get(),
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
			    d_proxySink.get(),
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
	        d_proxySink.get(),
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

	if (patternByName.count(infos.Location.toStdString()) == 0) {
		d_logger.Error(
		    "unknown pattern name",
		    slog::String("pattern", infos.Location.toStdString())
		);
		return;
	}
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
	        d_proxySink.get(),
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
