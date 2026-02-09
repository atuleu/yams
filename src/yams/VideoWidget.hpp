#pragma once

#include <yams/gstreamer/Memory.hpp>

#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWindow>

#include <gst/gl/gstgl_fwd.h>

#include <tuple>

typedef struct _GstGLSyncMeta GstGLSyncMeta;

namespace yams {

class VideoWidget : public QOpenGLWindow, protected QOpenGLFunctions {
	Q_OBJECT
public:
	VideoWidget(QScreen *target, QWindow *parent = nullptr);
	virtual ~VideoWidget();

	std::tuple<GstGLDisplay *, GstGLContext *> wrappedContext() const;

public slots:

	void pushNewFrame(quintptr frame, QSize size);

protected:
	void initializeGL() override;
	void paintGL() override;
	void resizeGL(int w, int h) override;

private:
	void setSize(int w, int h);

	struct Frame {
		GstVideoFramePtr VideoFrame = nullptr;
		QSize            Size;
		guint            TexID = 0;
		GstGLSyncMeta   *Sync  = nullptr;

		Frame();
		Frame(GstVideoFrame *frame, QSize size);
	};

	struct Matrix3f {
		float data[9];
	};

	Matrix3f computeProjection(const QSize &size) const;

	QSize                    d_size = {0, 0};
	QOpenGLVertexArrayObject d_frameVAO;
	QOpenGLBuffer            d_frameVBO;
	QOpenGLShaderProgram     d_shader;
	Frame                    d_frame;
	QOpenGLTexture          *d_placeholder = nullptr;
	std::atomic<bool>        d_initialized = false;
	GstGLDisplayPtr          d_display;
	GstGLContextPtr          d_context;
};
} // namespace yams
