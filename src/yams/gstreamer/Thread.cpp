#include "Thread.hpp"

#include <glib-object.h>
#include <glib.h>
#include <gst/gstbus.h>
#include <gst/gstmessage.h>
#include <memory>
#include <qmetaobject.h>
#include <qnamespace.h>

#include <qobject.h>
#include <qsocketnotifier.h>
#include <qthread.h>
#include <qtmetamacros.h>
#include <slog++/slog++.hpp>

namespace yams {

GstThreadLegacy::GstThreadLegacy(QObject *parent)
    : QThread{parent} {
#ifndef Q_OS_LINUX
	slog::Info("creating GMainLoop");
	d_loop = g_main_loop_new(nullptr, FALSE);
#endif
}

GstThreadLegacy::~GstThreadLegacy() {
#ifndef Q_OS_LINUX
	slog::Info("derefering GMainLoop");
	g_object_unref(d_loop);
#endif
}

void GstThreadLegacy::stop() {
#ifdef Q_OS_LINUX
	emit stopRequested();
#else
	g_main_loop_quit(d_loop);
#endif
}

void GstThreadLegacy::run() {
	slog::Info("starting GLib mainloop");
	startTask();
#ifdef Q_OS_LINUX
	// Runs a gmailoop on linux
	connect(this, &GstThreadLegacy::stopRequested, this, &QThread::quit);
	exec();
#else
	g_main_loop_run(d_loop);
#endif
	slog::Info("GLib mainloop done");

	doneTask();
}

} // namespace yams
