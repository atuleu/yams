#pragma once

#include <chrono>

#include <QObject>
#include <QSize>

#include <gst/gl/gstgl_fwd.h>
#include <gst/gstbus.h>
#include <gst/gstclock.h>
#include <gst/gstmemory.h>
#include <gst/gstpad.h>

#include <slog++/Logger.hpp>

#include "Frame.hpp"
#include "MediaPlayInfo.hpp"
#include <yams/gstreamer/Memory.hpp>
#include <yams/gstreamer/Pipeline.hpp>
#include <yams/utils/ObjectPool.hpp>

namespace yams {
using namespace std::chrono_literals;

class Compositor : public yams::Pipeline {
	struct LayerData;
	Q_OBJECT
public:
	struct Options {

		QSize                    Size    = {1920, 1080};
		size_t                   Layers  = 1;
		qreal                    FPS     = 60;
		std::chrono::nanoseconds Latency = 100ms;
	};

	struct Args {
		GstGLDisplay *Display;
		GstGLContext *Context;
		QObject      *Parent;
	};

	Compositor(Options options, Args args);
	virtual ~Compositor();

public slots:
	void start();
	void play(const MediaPlayInfo &media, int layer);
	void stop();

private slots:
	void playUnsafe(
	    const MediaPlayInfo &media, int layer, std::chrono::nanoseconds from
	);

	void removeMedia(LayerData *layer);
signals:
	void newFrame(yams::Frame::Ptr frame);
	void outputSizeChanged(QSize size);

protected:
	void            onMessage(GstMessage *msg) noexcept override;
	GstBusSyncReply onSyncMessage(GstMessage *msg) noexcept override;

private:
	static GstPadProbeReturn onSinkEventProbe(
	    GstPad *pad, GstPadProbeInfo *info, Compositor::LayerData *layer
	);

	static GstPadProbeReturn onBufferProbe(
	    GstPad *pad, GstPadProbeInfo *info, Compositor::LayerData *layer
	);

	static GstFlowReturn onNewSampleCb(GstElement *appsink, Compositor *self);

	void buildLayers(const Options &options);

	slog::Logger<1> d_logger;

	GstGLDisplay               *d_display;
	GstGLContext               *d_context;
	GstGLContextPtr             d_gstContext{nullptr};
	std::optional<GstVideoInfo> d_infos;

	GstElementPtr d_blacksrc, d_compositor;

	using FramePool = ObjectPool<Frame>;
	FramePool::Ptr d_pool;

	QSize                                   d_size;
	std::vector<std::unique_ptr<LayerData>> d_layers;
	GstClockPtr                             d_clock;
};

} // namespace yams
