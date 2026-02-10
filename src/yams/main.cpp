#include <QApplication>
#include <QObject>
#include <QScreen>
#include <QTimer>

#include "Frame.hpp"
#include "VideoOutput.hpp"
#include "VideoThread.hpp"
#include "yams/Compositor.hpp"

#include <gst/video/video-info.h>
#include <qobject.h>
#include <qthread.h>
#include <slog++/slog++.hpp>

int main(int argc, char *argv[]) {

	gst_init(&argc, &argv);
	QApplication app(argc, argv);

	qRegisterMetaType<yams::Frame::Ptr>();

	QSurfaceFormat fmt;
	fmt.setMajorVersion(3);
	fmt.setMinorVersion(3);
	fmt.setProfile(QSurfaceFormat::CoreProfile);
	fmt.setDepthBufferSize(8);
	fmt.setStencilBufferSize(8);
	fmt.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
	QSurfaceFormat::setDefaultFormat(fmt);

	auto screens = QGuiApplication::screens();
	if (screens.size() == 1) {
		slog::Warn("only one screen");
	}
	auto target = screens[std::min(qsizetype(1), screens.size() - 1)];
	slog::Info(
	    "target screen",
	    slog::String("manufacturer", target->manufacturer().toStdString()),
	    slog::String("model", target->model().toStdString())
	);

	yams::VideoOutput window{target};
	window.showOnTarget();
	QTimer::singleShot(6000, [&]() {
		slog::Info("stopping pipeline");
		window.close();
	});

	return app.exec();
}
