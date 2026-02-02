#pragma once

#include "yams/GstThread.hpp"
#include <qtmetamacros.h>

namespace yams {
namespace details {

template <typename T> struct GObjectUnrefer {
	void operator()(T *obj) const noexcept {
		if (obj != nullptr) {
			g_object_unref(obj);
		}
	}
};

template <typename T>
using glib_owned_ptr = std::unique_ptr<T, GObjectUnrefer<T>>;
using GstElementPtr  = glib_owned_ptr<GstElement>;
using GstCapsPtr     = glib_owned_ptr<GstCaps>;
using GstBusPtr      = glib_owned_ptr<GstBus>;
using GstMessagePtr  = glib_owned_ptr<GstMessage>;

} // namespace details

class VideoThread : public GstThread {
	Q_OBJECT
public:
	VideoThread(QObject *parent = nullptr);

	void stopPipeline();

protected:
	void startTask() override;
	void doneTask() override;
	void onNewFrame(GstBuffer *buffer);

	static void onNewFrameCb(
	    GstElement *fakesink, GstBuffer *buffer, GstPad *pad, gpointer userdata
	);
	static gboolean
	onNewMessageCb(GstBus *bus, GstMessage *msg, gpointer userdata);

signals:
	void newFrame(void *buffer);

private:
	details::GstElementPtr d_pipeline;
};

} // namespace yams
