#pragma once

#include "yams/gstreamer/Memory.hpp"
#include <chrono>

#include <QObject>
#include <QSize>

#include <gst/gl/gstgl_fwd.h>
#include <gst/gstbus.h>
#include <gst/gstpad.h>
#include <qobject.h>
#include <slog++/Logger.hpp>
#include <yams/gstreamer/Pipeline.hpp>

namespace yams {

struct MediaPlayInfo {
	enum class Type {
		VIDEO,
		IMAGE,
		TEST,
	};
	MediaPlayInfo::Type      Type;
	QString                  Location;
	std::chrono::nanoseconds Duration;
	std::chrono::nanoseconds Fade{0};
	bool                     Loop{false};
};
} // namespace yams

Q_DECLARE_METATYPE(yams::MediaPlayInfo);
Q_DECLARE_METATYPE(GstVideoInfo);

namespace yams {
using namespace std::chrono_literals;

class Compositor : public yams::Pipeline {
	Q_OBJECT
public:
	struct Options {

		QSize                    Size    = {1920, 1080};
		int                      Layers  = 1;
		int                      FPS     = 60;
		std::chrono::nanoseconds Latency = 10ms;
	};

	struct Args {
		GstGLDisplay *Display;
		GstGLContext *Context;
		QObject      *Parent;
	};

	Compositor(Options options, Args args);

public slots:
	void start(MediaPlayInfo media, int layer);

signals:
	void newFrame(quintptr frame);
	void videoInfos(GstVideoInfo infos);

protected:
	void            onMessage(GstMessage *msg) noexcept override;
	GstBusSyncReply onSyncMessage(GstMessage *msg) noexcept override;

private:
	static GstFlowReturn onNewSampleCb(GstElement *appsink, Compositor *self);

	slog::Logger<1> d_logger;

	GstGLDisplay               *d_display;
	GstGLContext               *d_context;
	GstGLContextPtr             d_gstContext{nullptr};
	std::optional<GstVideoInfo> d_infos;

	GstElementPtr d_blacksrc, d_compositor;
};
} // namespace yams
