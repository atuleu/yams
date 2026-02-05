#pragma once

#include "yams/GstThread.hpp"

#include <QObject>
#include <gst/gl/gstgl_fwd.h>

#include "yams/gstreamer.hpp"

namespace yams {
namespace details {} // namespace details

class VideoThread : public GstThread {
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

	static void onNewFrameCb(
	    GstElement *fakesink, GstBuffer *buffer, GstPad *pad, VideoThread *self
	);

	static gboolean
	onNewMessageCb(GstBus *bus, GstMessage *msg, gpointer userdata);

	static gboolean
	onSyncMessageCb(GstBus *bus, GstMessage *buffer, VideoThread *userdata);

signals:
	void newFrame(void *buffer);

private:
	GstElementPtr d_pipeline;
	GstGLDisplay *d_display;
	GstGLContext *d_context;
};

} // namespace yams
