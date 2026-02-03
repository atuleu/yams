#include "VideoWidget.hpp"

#include <QGuiApplication>
#include <QMetaEnum>
#include <QPainter>
#include <QScreen>

#include <gst/gst.h>

#include <qnamespace.h>
#include <qsurfaceformat.h>

#include <slog++/Attribute.hpp>
#include <slog++/slog++.hpp>

#include "defer.hpp"

namespace yams {
template <typename Str> slog::Attribute slogQRect(Str &&name, const QRect &r) {
	return slog::Group(
	    std::forward<Str>(name),
	    slog::Int("x", r.x()),
	    slog::Int("y", r.y()),
	    slog::Int("width", r.width()),
	    slog::Int("height", r.height())
	);
}

template <typename Str>
slog::Attribute slogQPoint(Str &&name, const QPoint &p) {
	return slog::Group(
	    std::forward<Str>(name),
	    slog::Int("x", p.x()),
	    slog::Int("y", p.y())
	);
}

template <typename Str, typename Enum>
slog::Attribute slogQEnum(Str &&name, Enum m) {
	QMetaEnum me = QMetaEnum::fromType<Enum>();
	if (me.isValid()) {
		return slog::String(
		    std::forward<Str>(name),
		    me.valueToKey(static_cast<int>(m))
		);
	} else {
		return slog::Int(std::forward<Str>(name), static_cast<int>(m));
	}
}

VideoWidget::VideoWidget(QWindow *parent)
    : QOpenGLWindow(QOpenGLWindow::NoPartialUpdate, parent) {

	auto defaultFmt = QSurfaceFormat::defaultFormat();

	auto screens = QGuiApplication::screens();
	if (screens.size() == 1) {
		slog::Warn("only one screen");
	}
	auto target = screens[std::min(qsizetype(1), screens.size() - 1)];

	setFlags(Qt::Window | Qt::FramelessWindowHint);
	setScreen(target);
	setGeometry(target->geometry());

	slog::Info(
	    "target screen",
	    slog::String("manufacturer", target->manufacturer().toStdString()),
	    slog::String("model", target->model().toStdString()),
	    slogQRect("geometry", target->geometry()),
	    slogQRect("virtualGeometry", target->virtualGeometry()),
	    slog::String("actual", screen()->model().toStdString())
	);

	auto displayFormat = [this](bool visible) {
		if (visible == false) {
			return;
		}
		auto actualFormat = this->format();
		slog::Info(
		    "windows format",
		    slog::Int("major", actualFormat.majorVersion()),
		    slog::Int("minor", actualFormat.majorVersion()),
		    slogQEnum("profile", actualFormat.profile()),
		    slogQEnum("swapBehavior", actualFormat.swapBehavior()),
		    slog::Int("depthBufferSize", actualFormat.depthBufferSize()),
		    slog::Int("stencilBufferSize", actualFormat.stencilBufferSize())
		);
	};

	connect(this, &QWindow::visibleChanged, this, displayFormat);

	showFullScreen();
}

VideoWidget::~VideoWidget() {}

void VideoWidget::pushNewBuffer(void *buffer) {
	auto buffer_ = reinterpret_cast<GstBuffer *>(buffer);
	// discard it for now.
	gst_buffer_unref(buffer_);
	update();
}

void VideoWidget::initializeGL() {}

void VideoWidget::paintGL() {}

void VideoWidget::resizeGL(int w, int h) {}

} // namespace yams
