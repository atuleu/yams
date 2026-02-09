#pragma once

#include <QThread>

#include <glib.h>
#include <gst/gst.h>

#include <QSocketNotifier>

#include <yams/gstreamer/Memory.hpp>

namespace yams {

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
