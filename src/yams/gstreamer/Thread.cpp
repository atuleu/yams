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

namespace details {
class BusWatcher : public QObject {
	Q_OBJECT
public:
	BusWatcher(GstBus *bus, QThread *parent)
	    : d_bus{bus} {
		GPollFD pollFD;
		gst_bus_get_pollfd(bus, &pollFD);
		slog::DDebug(
		    "GstBusWatcher got GPollFD",
		    slog::Int("FD", pollFD.fd),
		    slog::Int("events", pollFD.events),
		    slog::Int("revents", pollFD.revents)
		);
		if ((pollFD.events & G_IO_IN) != 0) {
			addNotifier(pollFD.fd, QSocketNotifier::Read);
		}
		if ((pollFD.events & G_IO_OUT) != 0) {
			addNotifier(pollFD.fd, QSocketNotifier::Write);
		}
		if ((pollFD.events & G_IO_ERR) != 0) {
			addNotifier(pollFD.fd, QSocketNotifier::Exception);
		}
		moveToThread(parent);
	}

	void setCallback(GstBusFunc callback, gpointer userdata) {
		if (callback == nullptr) {
			if (d_callback != nullptr) {
				for (auto n : d_notifiers) {
					disconnect(
					    n,
					    &QSocketNotifier::activated,
					    this,
					    &BusWatcher::pollBus
					);
					QMetaObject::invokeMethod(
					    n,
					    &QSocketNotifier::setEnabled,
					    Qt::QueuedConnection,
					    false
					);
				}
			}
			d_callback = nullptr;
			d_userdata = nullptr;
			return;
		}
		d_callback = callback;
		d_userdata = userdata;
		for (auto n : d_notifiers) {
			connect(n, &QSocketNotifier::activated, this, &BusWatcher::pollBus);
			QMetaObject::invokeMethod(
			    n,
			    &QSocketNotifier::setEnabled,
			    Qt::QueuedConnection,
			    true
			);
		}
	}

protected slots:

	void pollBus() {
		while (true) {
			auto msg = gst_bus_pop(d_bus);
			if (msg == nullptr) {
				return;
			}
			d_callback(d_bus, msg, d_userdata);
		}
	}

private:
	void addNotifier(gint fd, QSocketNotifier::Type type) {
		auto n = d_notifiers.emplace_back(new QSocketNotifier(fd, type, this));
		slog::DDebug(
		    "GstBusWatcher added notifier",
		    slog::Int("FD", fd),
		    slog::Int("type", int(type))
		);
	}

	std::vector<QSocketNotifier *> d_notifiers;
	GstBus                        *d_bus;
	GstBusFunc                     d_callback{nullptr};
	gpointer                       d_userdata{nullptr};
};
} // namespace details

GstThread::GstThread(QObject *parent)
    : QThread{parent} {};
GstThread::~GstThread() = default;

void GstThread::watchBus(GstBus *bus, GstBusFunc callback, gpointer userdata) {
	if (callback == nullptr) {
		unwatchBus(bus);
		return;
	}
	if (d_buses.count(bus) == 0) {
		d_buses.emplace(bus, std::make_unique<details::BusWatcher>(bus, this));
	}
	d_buses.at(bus)->setCallback(callback, userdata);
}

void GstThread::unwatchBus(GstBus *bus) {
	d_buses.erase(bus);
}

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

#include "Thread.moc"
