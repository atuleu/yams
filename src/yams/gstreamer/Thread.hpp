#pragma once

#include <QThread>

#include <glib.h>
#include <gst/gst.h>

#include <QSocketNotifier>

#include <yams/gstreamer/memory.hpp>

namespace yams {

namespace details {
class BusWatcher;
}

class GstThread : public QThread {
	Q_OBJECT
public:
	GstThread(QObject *parent = nullptr);
	virtual ~GstThread();
	GstThread(const GstThread &)            = delete;
	GstThread(GstThread &&)                 = delete;
	GstThread &operator=(const GstThread &) = delete;
	GstThread &operator=(GstThread &&)      = delete;

	void watchBus(GstBus *bus, GstBusFunc callback, gpointer userdata);
	void unwatchBus(GstBus *bus);

private:
	std::map<GstBus *, std::unique_ptr<details::BusWatcher>> d_buses;
};

class GstThreadLegacy : public QThread {
	Q_OBJECT
public:
	GstThreadLegacy(QObject *parent = nullptr);
	virtual ~GstThreadLegacy();
	GstThreadLegacy(const GstThreadLegacy &)            = delete;
	GstThreadLegacy(GstThreadLegacy &&)                 = delete;
	GstThreadLegacy &operator=(const GstThreadLegacy &) = delete;
	GstThreadLegacy &operator=(GstThreadLegacy &&)      = delete;

	void stop();

#ifdef Q_OS_LINUX
signals:
	void stopRequested();
#endif

protected:
	void         run() override;
	virtual void startTask() = 0;
	virtual void doneTask()  = 0;

#ifndef Q_OS_LINUX
	GMainLoop *d_loop;
#endif

	std::function<void()> d_start, d_done;
};

} // namespace yams
