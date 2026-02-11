#include "VideoOutput.hpp"
#include "yams/Compositor.hpp"

#include <QGuiApplication>
#include <QMetaEnum>
#include <QOpenGLFunctions>
#include <QPainter>
#include <QScreen>
#include <QTimer>

#include <glib.h>
#include <gst/gl/gl.h>
#include <gst/gl/gstglcontext.h>
#include <gst/gst.h>
#include <gst/gstbuffer.h>
#include <gst/video/video-frame.h>
#include <gst/video/video-info.h>

#include <memory>
#include <qcoreapplication.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qobjectdefs.h>
#include <qopenglwindow.h>
#include <qwindow.h>
#include <yams/gstreamer/Memory.hpp>
#include <yams/gstreamer/QOpenGL.hpp>
#include <yams/utils/slogQt.hpp>

#include <gst/video/video.h>

namespace yams {

void VideoOutput::pushNewFrame(yams::Frame::Ptr frame) {
	d_frame = frame;
	update();
}

VideoOutput::VideoOutput(QScreen *target, QWindow *parent)
    : QOpenGLWindow(QOpenGLWindow::NoPartialUpdate, parent)
    , d_size{target->geometry().size()}
    , d_inputSize{target->geometry().size()} {

	setCursor(QCursor{Qt::BlankCursor});
	setFlags(Qt::Window | Qt::FramelessWindowHint);
	setScreen(target);
	setGeometry(target->geometry());

#ifndef NDEBUG
	auto displayFormat = [this, target](bool visible) {
		if (visible == false) {
			return;
		}
		auto actualFormat = this->format();
		slog::Info(
		    "windows format",
		    slog::Int("major", actualFormat.majorVersion()),
		    slog::Int("minor", actualFormat.majorVersion()),
		    slog::QEnum("profile", actualFormat.profile()),
		    slog::QEnum("swapBehavior", actualFormat.swapBehavior()),
		    slog::Int("depthBufferSize", actualFormat.depthBufferSize()),
		    slog::Int("stencilBufferSize", actualFormat.stencilBufferSize())
		);
	};

	connect(this, &QWindow::visibleChanged, this, displayFormat);
#endif
	d_projection = computeProjection();
}

void VideoOutput::showOnTarget() {
	show();
	QCoreApplication::processEvents();
	QMetaObject::invokeMethod(
	    this,
	    &QOpenGLWindow::showFullScreen,
	    Qt::QueuedConnection
	);
}

void VideoOutput::closeEvent(QCloseEvent *event) {
	d_compositor.reset();
	QOpenGLWindow::closeEvent(event);
}

VideoOutput::~VideoOutput() {
	d_compositor.reset();
	d_gstreamerThread.quit();
	d_gstreamerThread.wait();
}

void VideoOutput::initializeGL() {
	initializeOpenGLFunctions();

	d_display = fromGuiApplication();
	d_context = wrapQOpenGLContext(d_display.get(), context());

	gst_gl_context_activate(d_context.get(), TRUE);
	GError *error{nullptr};
	if (gst_gl_context_fill_info(d_context.get(), &error) == false ||
	    error != nullptr) {
		slog::Fatal(
		    "could not fill info",
		    slog::String("error", error != nullptr ? error->message : "unknown")
		);
		if (error != nullptr) {
			g_error_free(error);
		}
	}

	d_compositor = std::make_unique<Compositor>(
	    Compositor::Options{
	        .Size   = screen()->geometry().size(),
	        .Layers = 2,
	        .FPS    = screen()->refreshRate(),
	    },
	    Compositor::Args{
	        .Display = d_display.get(),
	        .Context = d_context.get(),
	        .Parent  = nullptr,
	    }
	);
	d_compositor->moveToThread(&d_gstreamerThread);
	d_gstreamerThread.start();
	connect(
	    d_compositor.get(),
	    &Compositor::outputSizeChanged,
	    this,
	    &VideoOutput::updateWorkingSize,
	    Qt::QueuedConnection
	);
	connect(
	    d_compositor.get(),
	    &Compositor::newFrame,
	    this,
	    &VideoOutput::pushNewFrame,
	    Qt::QueuedConnection
	);

	d_compositor->start();

	d_initialized.store(true);
	d_initialized.notify_all();

	glEnable(GL_TEXTURE_2D);

	glViewport(0, 0, d_size.width(), d_size.height());

	d_shader.addShaderFromSourceFile(
	    QOpenGLShader::Vertex,
	    ":shaders/frame.vertex"
	);
	d_shader.addShaderFromSourceFile(
	    QOpenGLShader::Fragment,
	    ":shaders/frame.fragment"
	);
	d_shader.link();

	d_frameVAO.create();
	d_frameVBO.create();
	d_frameVAO.bind();
	d_frameVBO.bind();
	float frameVertices[24] = {
	    -1.0f, -1.0f, 0.0f, 0.0f, //
	    +1.0f, -1.0f, 1.0f, 0.0f, //
	    +1.0f, +1.0f, 1.0f, 1.0f, //
	    +1.0f, +1.0f, 1.0f, 1.0f, //
	    -1.0f, 1.0f,  0.0f, 1.0f, //
	    -1.0f, -1.0f, 0.0f, 0.0f, //
	};
	glBufferData(
	    GL_ARRAY_BUFFER,
	    sizeof(frameVertices),
	    frameVertices,
	    GL_STATIC_DRAW
	);
	glVertexAttribPointer(
	    0,
	    2,
	    GL_FLOAT,
	    GL_FALSE,
	    4 * sizeof(float),
	    (void *)(0 * sizeof(float))
	);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(
	    1,
	    2,
	    GL_FLOAT,
	    GL_FALSE,
	    4 * sizeof(float),
	    (void *)(2 * sizeof(float))
	);
	glEnableVertexAttribArray(1);
	d_frameVBO.release();
	d_frameVAO.release();

	QSize textureSize = {320, 240};
	d_inputSize       = textureSize;
	QImage frame{textureSize, QImage::Format_RGB888};
	frame.fill(Qt::cyan);

	d_placeholder = new QOpenGLTexture(frame);
	d_projection  = computeProjection();
}

VideoOutput::Matrix3f VideoOutput::computeProjection() const {
	auto  ratio = float(d_size.height()) / float(d_inputSize.height());
	float width = d_inputSize.width() * ratio;
	if (width <= d_size.width()) {
		ratio = float(width) / float(d_size.width());
		// viewport with full height
		// clang-format off
		return Matrix3f{
		    .data = {
				ratio, 0.0f, 0.0f,
				0.0f, -1.0f, 0.0f,
		        0.0f, 0.0f, 0.0f
			},
		};
		// clang-format on
	} else {
		float height = d_inputSize.height() * float(d_size.width()) /
		               float(d_inputSize.width());
		ratio = float(height) / float(d_size.height());
		// viewport with full width
		// clang-format off
		return Matrix3f{
		    .data = {
				1.0f, 0.0f, 0.0f,
				0.0f, -ratio, 0.0f,
		        0.0f, 0.0f, 0.0f
			},
		};
		// clang-format on
	}
}

void VideoOutput::updateWorkingSize(QSize size) {
	d_inputSize  = size;
	d_projection = computeProjection();
	update();
}

void VideoOutput::paintGL() {
	while (d_toDispose.size() > 5) {
		d_toDispose.pop_front();
	}

	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	d_shader.bind();

	auto loc = d_shader.uniformLocation("scaleMat");
	glUniformMatrix3fv(loc, 1, GL_FALSE, d_projection.data);

	glActiveTexture(GL_TEXTURE0);
	if (d_frame != nullptr) {
		d_frame->waitSync(d_context.get());
		glBindTexture(GL_TEXTURE_2D, d_frame->TexID());
	} else {
		d_placeholder->bind();
	}

	d_toDispose.push_back(d_frame);

	d_frameVAO.bind();
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

void VideoOutput::resizeGL(int w, int h) {
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
	d_projection = computeProjection();
	update();
}

Compositor *VideoOutput::compositor() {
	d_initialized.wait(false);
	return d_compositor.get();
}

} // namespace yams
