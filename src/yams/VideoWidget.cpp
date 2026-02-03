#include "VideoWidget.hpp"

#include <QGuiApplication>
#include <QMetaEnum>
#include <QOpenGLFunctions>
#include <QPainter>
#include <QScreen>
#include <QTimer>

#include <gst/gst.h>

#include <qnamespace.h>
#include <qopenglext.h>
#include <qopenglshaderprogram.h>
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

	setCursor(QCursor{Qt::BlankCursor});
	setFlags(Qt::Window | Qt::FramelessWindowHint);
	setScreen(target);
	setGeometry(target->geometry());

	slog::Info(
	    "target screen",
	    slog::String("manufacturer", target->manufacturer().toStdString()),
	    slog::String("model", target->model().toStdString())
	);

	auto displayFormat = [this, target](bool visible) {
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

	// we need to show, processEvents and make fullscreen to avoid race
	// conditions in cosmic-comp. Your Mileage may not work.
	show();

	QCoreApplication::processEvents();
	QTimer::singleShot(0, this, [this]() { showFullScreen(); });
}

VideoWidget::~VideoWidget() {}

void VideoWidget::pushNewBuffer(void *buffer) {
	auto buffer_ = reinterpret_cast<GstBuffer *>(buffer);
	// discard it for now.
	gst_buffer_unref(buffer_);
	update();
}

void VideoWidget::initializeGL() {
	initializeOpenGLFunctions();

	glViewport(0, 0, d_size.width(), d_size.height());
	d_triangleVBO.create();
	d_triangleVAO.create();
	d_triangleVAO.bind();
	d_triangleVBO.bind();
	// clang-format off
	float vertices[] = {
		// positions         // colors
		0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,   // bottom right
		-0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,   // bottom left
		0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f    // top
	};
	// clang-format on
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(
	    0,
	    3,
	    GL_FLOAT,
	    GL_FALSE,
	    6 * sizeof(float),
	    (void *)0
	);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(
	    1,
	    3,
	    GL_FLOAT,
	    GL_FALSE,
	    6 * sizeof(float),
	    (void *)(3 * sizeof(float))
	);
	glEnableVertexAttribArray(1);

	d_triangleVBO.release();
	d_triangleVAO.release();

	d_shader.addShaderFromSourceFile(
	    QOpenGLShader::Vertex,
	    ":shaders/sprite.vertex"
	);
	d_shader.addShaderFromSourceFile(
	    QOpenGLShader::Fragment,
	    ":shaders/sprite.fragment"
	);
	d_shader.link();
}

void VideoWidget::paintGL() {
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	d_shader.bind();
	d_triangleVAO.bind();
	glDrawArrays(GL_TRIANGLES, 0, 3);
}

template <typename Str>
slog::Attribute slogQSize(Str &&name, const QSize &size) {
	return slog::Group(
	    std::forward<Str>(name),
	    slog::Int("width", size.width()),
	    slog::Int("height", size.height())
	);
}

void VideoWidget::resizeGL(int w, int h) {
	auto newSize = QSize{w, h};
	if (d_size == newSize) {
		return;
	}
	d_size = newSize;
	slog::Info(
	    "resize",
	    slog::Group("size", slog::Int("width", w), slog::Int("height", h))
	);
	glViewport(0, 0, w, h);
	update();
}

} // namespace yams
