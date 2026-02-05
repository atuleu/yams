#include <QApplication>
#include <QObject>
#include <QScreen>
#include <QTimer>

#include "VideoThread.hpp"
#include "VideoWidget.hpp"

#include <slog++/slog++.hpp>

int main(int argc, char *argv[]) {

	gst_init(&argc, &argv);
	QApplication app(argc, argv);

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

	yams::VideoWidget window{target};

	auto [display, context] = window.wrappedContext();

	yams::VideoThread videoTask{display, context};

	// QObject::connect(
	//     &videoTask,
	//     SIGNAL(newFrame(void *)),
	//     &window,
	//     SLOT(pushNewBuffer(void *)),
	//     Qt::QueuedConnection
	// );

	QObject::connect(&videoTask, SIGNAL(finished()), &window, SLOT(close()));

	window.show();

	videoTask.start();
	QTimer::singleShot(6000, [&]() {
		slog::Info("stopping pipeline");
		videoTask.stopPipeline();
	});

	return app.exec();
}
