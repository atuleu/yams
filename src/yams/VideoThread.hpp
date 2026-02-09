#pragma once

#include <yams/gstreamer/Thread.hpp>

#include <QObject>
#include <QSize>

#include <yams/gstreamer/memory.hpp>

namespace yams {
namespace details {} // namespace details

class VideoThread : public GstThreadLegacy {
	Q_OBJECT
public:
	VideoThread(
	    GstGLDisplay *display, GstGLContext *context, QObject *parent = nullptr
	);

	void stopPipeline();

protected:
	void startTask() override;
	void doneTask() override;
	void onNewFrame(GstBuffer *buffer);

	GstFlowReturn onNewSample(GstElement *appsink);

	static GstFlowReturn onNewSampleCb(GstElement *appsink, VideoThread *self);

	static gboolean
	onNewMessageCb(GstBus *bus, GstMessage *msg, gpointer userdata);

	static gboolean
	onSyncMessageCb(GstBus *bus, GstMessage *buffer, VideoThread *userdata);

signals:
	void newFrame(quintptr frame, QSize size);

private:
	GstElementPtr               d_pipeline;
	GstGLDisplay               *d_display;
	GstGLContext               *d_context;
	GstGLContextPtr             d_gstContext{nullptr};
	std::optional<GstVideoInfo> d_infos;
};

} // namespace yams
