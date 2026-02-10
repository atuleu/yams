#pragma once

#include "yams/Compositor.hpp"
#include "yams/Frame.hpp"
#include <yams/gstreamer/Memory.hpp>

#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWindow>

#include <gst/gl/gstgl_fwd.h>

#include <tuple>

namespace yams {

class VideoOutput : public QOpenGLWindow, protected QOpenGLFunctions {
	Q_OBJECT
public:
	VideoOutput(QScreen *target, QWindow *parent = nullptr);
	virtual ~VideoOutput();

	VideoOutput(const VideoOutput &)            = delete;
	VideoOutput(VideoOutput &&)                 = delete;
	VideoOutput &operator=(const VideoOutput &) = delete;
	VideoOutput &operator=(VideoOutput &&)      = delete;

	Compositor *compositor();

public slots:
	void showOnTarget();
	void pushNewFrame(yams::Frame::Ptr frame);
	void updateWorkingSize(QSize size);

protected:
	void initializeGL() override;
	void paintGL() override;
	void resizeGL(int w, int h) override;

	void closeEvent(QCloseEvent *event) override;

private:
	void setSize(int w, int h);

	struct Matrix3f {
		float data[9];
	};

	Matrix3f computeProjection(const QSize &size) const;

	QThread     d_gstreamerThread;
	std::unique_ptr<Compositor> d_compositor;

	QSize                    d_size = {0, 0};
	QOpenGLVertexArrayObject d_frameVAO;
	QOpenGLBuffer            d_frameVBO;
	QOpenGLShaderProgram     d_shader;
	Frame::Ptr               d_frame;
	QOpenGLTexture          *d_placeholder = nullptr;
	std::atomic<bool>        d_initialized = false;
	GstGLDisplayPtr          d_display;
	GstGLContextPtr          d_context;
	Matrix3f                 d_projection;
};
} // namespace yams
