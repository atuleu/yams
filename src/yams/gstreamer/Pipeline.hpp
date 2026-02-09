#pragma once

#include <qtmetamacros.h>
#include <yams/gstreamer/Memory.hpp>
#include <yams/gstreamer/Thread.hpp>

namespace yams {
class Pipeline : public QObject {
	Q_OBJECT
public:
	Pipeline(const char *name, GstThread *thread, QObject *parent);
	virtual ~Pipeline();
	Pipeline(const Pipeline &)            = delete;
	Pipeline(Pipeline &&)                 = delete;
	Pipeline &operator=(const Pipeline &) = delete;
	Pipeline &operator=(Pipeline &&)      = delete;

protected:
	virtual void onMessage(GstMessage *msg) = 0;

protected:
	static gboolean onMessageCb(GstBus *bus, GstMessage *msg, Pipeline *self);
	GstThread      *d_thread;
	GstElementPtr   d_pipeline;
	GstBusPtr       d_bus;
};
} // namespace yams
