#include <gst/gst.h>
#include <gst/gstelement.h>
#include <gst/video/videooverlay.h>

#include <QApplication>
#include <QLabel>
#include <QMainWindow>
#include <QTimer>

#include <slog++/slog++.hpp>

#include "Version.hpp"
#include "git.h"

#define y_DEFER_UNIQUE_NAME_INNER(a, b) a##b
#define y_DEFER_UNIQUE_NAME(base, line) y_DEFER_UNIQUE_NAME_INNER(base, line)
#define y_DEFER_NAME                    y_DEFER_UNIQUE_NAME(zz_defer, __LINE__)

#define defer auto y_DEFER_NAME = details::Defer_void{} *[&]()

namespace details {
template <typename Lambda> struct Deferrer {
	Lambda lambda;

	~Deferrer() {
		lambda();
	};
};

struct Defer_void {};

template <typename Lambda>
Deferrer<Lambda> operator*(Defer_void, Lambda &&lambda) {
	return {lambda};
}

} // namespace details

template <typename T> struct GObjectUnrefer {
	void operator()(T *obj) const noexcept {
		if (obj != nullptr) {
			g_object_unref(obj);
		}
	}
};

template <typename T> struct GstBufferUnrefer {
	void operator()(T *obj) const noexcept {
		if (obj != nullptr) {
			gst_buffer_unref(obj);
		}
	}
};

template <typename T>
using glib_owned_ptr = std::unique_ptr<T, GObjectUnrefer<T>>;
using GstElementPtr  = glib_owned_ptr<GstElement>;

int main(int argc, char *argv[]) {
	gst_init(&argc, &argv);
	QApplication app(argc, argv);
	app.setQuitOnLastWindowClosed(true);

	auto pipeline = GstElementPtr{gst_pipeline_new("overlayed")};
	auto source   = gst_element_factory_make("videotestsrc", "source0");
	auto sink     = gst_element_factory_make("glimagesink", "sink0");

	if (sink == nullptr) {
		slog::Fatal("could not create imagesink");
	}

	gst_bin_add_many(GST_BIN(pipeline.get()), source, sink, nullptr);
	gst_element_link(source, sink);

	slog::Info(
	    "Starting YAMS",
	    slog::String("version", yams::Version()),
	    slog::String("SHA", git_CommitSHA1())
	);

	QMainWindow window;
	window.setWindowTitle(
	    ("YAMS: Yet Another Media Server " + yams::Version()).c_str()
	);
	window.resize(640, 480);
	window.show();
	defer {
		window.hide();
	};

	slog::Info("Main window displayed", slog::Int("winID", window.winId()));

	gst_video_overlay_set_window_handle(
	    GST_VIDEO_OVERLAY(sink),
	    window.winId()
	);

	slog::Info("starting pipeline");
	auto sret = GST_STATE_CHANGE_SUCCESS;
	// right now starting the pipeline makes everything crash with a segfault.
	// gst_element_set_state(pipeline.get(), GST_STATE_PLAYING);
	slog::Info("pipeline started?");
	if (sret == GST_STATE_CHANGE_FAILURE) {
		slog::Error("Could not set pipeline playing");
		gst_element_set_state(pipeline.get(), GST_STATE_NULL);
		// quit immediatly.

		QTimer::singleShot(0, [&] { window.close(); });
	}

	defer {
		gst_element_set_state(pipeline.get(), GST_STATE_NULL);
	};

	return app.exec();
}
