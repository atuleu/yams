#pragma once

#include <QThread>

#include <gst/gst.h>

namespace yams {
class GstThread : public QThread {
	Q_OBJECT
public:
	GstThread(QObject *parent = nullptr);
	virtual ~GstThread();
	GstThread(const GstThread &)            = delete;
	GstThread(GstThread &&)                 = delete;
	GstThread &operator=(const GstThread &) = delete;
	GstThread &operator=(GstThread &&)      = delete;

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
