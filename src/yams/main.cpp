#include <QApplication>
#include <QGuiApplication>
#include <QObject>
#include <QOpenGLContext>
#include <QScreen>
#include <QTimer>

#include "Frame.hpp"
#include "VideoOutput.hpp"
#include "yams/Compositor.hpp"
#include "yams/utils/slogQt.hpp"

#include <algorithm>
#include <chrono>
#include <gst/video/video-info.h>
#include <qcoreapplication.h>
#include <qmetaobject.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qsurfaceformat.h>
#include <qthread.h>
#include <qwindow.h>
#include <slog++/Attribute.hpp>
#include <slog++/slog++.hpp>

QScreen *selectScreen() {
	auto     screens = QGuiApplication::screens();
	QScreen *target;
	if (screens.size() == 1) {
		slog::Warn("only one screen");
		return screens[0];
	}
	auto wanted = std::getenv("YAMS_OUTPUT_SCREEN");

	if (wanted != nullptr) {
		auto wantedIdx = std::atoi(wanted);
		if (wantedIdx > 0 || QString(wanted) == "0") {
			if (wantedIdx >= screens.size()) {
				slog::Warn(
				    "screen index out of range",
				    slog::String("wanted", wanted)
				);
			} else {
				return screens[wantedIdx];
			}
		} else {

			auto found = std::find_if(
			    screens.begin(),
			    screens.end(),
			    [wanted = QString(wanted)](QScreen *s) {
				    return s->name() == wanted || s->model() == wanted;
			    }
			);
			if (found == screens.end()) {
				slog::Warn(
				    "wanted screen not found",
				    slog::String("wanted", wanted)
				);
			} else {
				return *found;
			}
		}
	}
	auto primaryScreen = QGuiApplication::primaryScreen();
	slog::Info("primary screen", slog::QScreen("primary", *primaryScreen));

	return *std::find_if(
	    screens.begin(),
	    screens.end(),
	    [primaryScreen](QScreen *s) {
		    return s->name() != primaryScreen->name();
	    }
	);
}

template <typename T> std::string enumName(T v) {
	auto me = QMetaEnum::fromType<T>();
	return me.valueToKey(v);
}

void setOpenGLFormat() {
	QOpenGLContext test;
	test.create();

	QSurfaceFormat fmt = QSurfaceFormat::defaultFormat();
	if (test.isOpenGLES()) {
		fmt.setRenderableType(QSurfaceFormat::OpenGLES);
		fmt.setMajorVersion(3);
		fmt.setMinorVersion(0);
	} else {
		fmt.setRenderableType(QSurfaceFormat::OpenGL);
		fmt.setMajorVersion(3);
		fmt.setMinorVersion(3);
		fmt.setProfile(QSurfaceFormat::CoreProfile);
	}
	QSurfaceFormat::setDefaultFormat(fmt);
	slog::Info(
	    "selected target surface format",
	    slog::QSurfaceFormat("test", test.format()),
	    slog::QSurfaceFormat("target", fmt)
	);
}

int main(int argc, char *argv[]) {

	gst_init(&argc, &argv);
	QGuiApplication app(argc, argv);
	if (std::getenv("YAMS_DEBUG") != nullptr) {
		slog::DefaultLogger().From(slog::Level::Debug);
	}
	setOpenGLFormat();

	qRegisterMetaType<yams::Frame::Ptr>();
	qRegisterMetaType<std::chrono::nanoseconds>();
	qRegisterMetaType<yams::MediaPlayInfo>();

	auto target = selectScreen();
	slog::Info(
	    "target screen",
	    slog::String("name", target->name().toStdString()),
	    slog::String("manufacturer", target->manufacturer().toStdString()),
	    slog::String("model", target->model().toStdString())
	);

	yams::VideoOutput window{target};
	if (QGuiApplication::screens().size() == 1) {
		window.show();
	} else {
		window.showOnTarget();
	}
	using namespace std::chrono_literals;
	QTimer::singleShot(2500, [&]() {
		slog::Info("playing ball pattern");
		window.compositor()->play(
		    yams::MediaPlayInfo{
		        .MediaType = yams::MediaPlayInfo::Type::TEST,
		        .Location  = "ball",
		        .Duration  = 2s,
		        .Fade      = 500ms,
		        .Loop      = true,
		    },
		    0
		);
	});

	QTimer::singleShot(3500, [&]() {
		slog::Info("playing smtpe pattern");
		window.compositor()->play(
		    yams::MediaPlayInfo{
		        .MediaType = yams::MediaPlayInfo::Type::TEST,
		        .Location  = "smtpe",
		        .Duration  = 2s,
		        .Fade      = 500ms,
		        .Loop      = false,
		    },
		    1
		);
	});

	QTimer::singleShot(8000, [&]() {
		slog::Info("stopping");
		window.close();
	});

	return app.exec();
}
