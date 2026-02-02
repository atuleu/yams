#include "GstThread.hpp"
#include <glib-object.h>
#include <qnamespace.h>

#include <slog++/slog++.hpp>

namespace yams {

GstThread::GstThread(QObject *parent)
    : QThread{parent} {
#ifndef Q_OS_LINUX
	slog::Info("creating GMainLoop");
	d_loop = g_main_loop_new(nullptr, FALSE);
#endif
}

GstThread::~GstThread() {
#ifndef Q_OS_LINUX
	slog::Info("derefering GMainLoop");
	g_object_unref(d_loop);
#endif
}

void GstThread::stop() {
#ifdef Q_OS_LINUX
	emit stopRequested();
#else
	g_main_loop_quit(d_loop);
#endif
}

void GstThread::run() {
	slog::Info("starting GLib mainloop");
	startTask();
#ifdef Q_OS_LINUX
	// Runs a gmailoop on linux
	connect(this, &GstThread::stopRequested, this, &QThread::quit);
	exec();
#else
	g_main_loop_run(d_loop);
#endif
	slog::Info("GLib mainloop done");

	doneTask();
}

} // namespace yams
