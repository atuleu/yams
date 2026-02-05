#include "VideoWidget.hpp"

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
#include <gst/video/video-info.h>

#include "slogQt.hpp"
#include "yams/gstQOpenGL.hpp"
#include "yams/gstreamer.hpp"

#include <gst/video/video.h>

namespace yams {
VideoWidget::Frame::Frame() {}

VideoWidget::Frame::Frame(GstBuffer *buffer)
    : Buffer{buffer} {
	if (buffer == nullptr) {
		return;
	}

	auto mem = gst_buffer_peek_memory(buffer, 0);
	if (gst_is_gl_memory(mem) == false) {
		throw std::runtime_error("buffer is not a GL mapped memory!");
	}

	Sync = gst_buffer_get_gl_sync_meta(buffer);
	if (Sync == nullptr) {
		throw std::runtime_error("buffer does not contains synchronization data"
		);
	}

	Memory    = reinterpret_cast<GstGLMemory *>(mem);
	auto meta = gst_buffer_get_video_meta(buffer);
	Size      = {int(meta->width), int(meta->height)};

	GstVideoInfo infos;
	gst_video_info_set_format(&infos, meta->format, meta->width, meta->height);
	GstVideoFrame frame;
	gst_video_frame_map(
	    &frame,
	    &infos,
	    buffer,
	    (GstMapFlags)(GST_MAP_READ | GST_MAP_GL)
	);
	TexID = *(guint *)frame.data[0];
}

void VideoWidget::pushNewBuffer(void *buffer) {
	if (buffer == nullptr) {
		return;
	}
	try {
		d_frame = Frame{reinterpret_cast<GstBuffer *>(buffer)};
	} catch (std::exception &e) {
		slog::Error("could not use buffer", slog::String("error", e.what()));
	}
	update();
}

VideoWidget::VideoWidget(QScreen *target, QWindow *parent)
    : QOpenGLWindow(QOpenGLWindow::NoPartialUpdate, parent) {

	setCursor(QCursor{Qt::BlankCursor});
	setFlags(Qt::Window | Qt::FramelessWindowHint);
	setScreen(target);
	setGeometry(target->geometry());

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

	// we need to show, processEvents and make fullscreen to avoid race
	// conditions in cosmic-comp. Your Mileage May Vary.
	show();
	QCoreApplication::processEvents();
	QTimer::singleShot(0, this, [this]() { showFullScreen(); });
}

VideoWidget::~VideoWidget() {}

void VideoWidget::initializeGL() {
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

	QSize  textureSize = {1680, 720};
	QImage frame{textureSize, QImage::Format_RGB888};
	frame.fill(Qt::cyan);

	d_placeholder = new QOpenGLTexture(frame);
	d_frame.Size  = textureSize;
}

VideoWidget::Matrix3f VideoWidget::computeProjection(const QSize &size) const {
	auto  ratio = float(d_size.height()) / float(size.height());
	float width = size.width() * ratio;
	if (width <= d_size.width()) {
		// viewport with full height
		// clang-format off
		return Matrix3f{
		    .data = {
				1.0f/ratio, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f,
		        0.0f, 0.0f, 0.0f
			},
		};
		// clang-format on
	} else {
		ratio = float(d_size.width()) / float(size.width());
		// viewport with full width
		// clang-format off
		return Matrix3f{
		    .data = {
				1.0f, 0.0f, 0.0f,
				0.0f, 1.0f/ratio, 0.0f,
		        0.0f, 0.0f, 0.0f
			},
		};
		// clang-format on
	}
}

void VideoWidget::paintGL() {
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	d_shader.bind();

	auto loc = d_shader.uniformLocation("scaleMat");
	glUniformMatrix3fv(loc, 1, GL_FALSE, computeProjection(d_frame.Size).data);

	glActiveTexture(GL_TEXTURE0);
	if (d_frame.Buffer != nullptr) {
		gst_gl_sync_meta_wait(d_frame.Sync, d_frame.Memory->mem.context);
		glBindTexture(GL_TEXTURE_2D, d_frame.TexID);
	} else {
		d_placeholder->bind();
	}
	d_frameVAO.bind();
	glDrawArrays(GL_TRIANGLES, 0, 6);
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

std::tuple<GstGLDisplay *, GstGLContext *> VideoWidget::wrappedContext() const {
	d_initialized.wait(false);
	return {d_display.get(), d_context.get()};
}
} // namespace yams
