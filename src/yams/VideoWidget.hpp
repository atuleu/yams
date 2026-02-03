#pragma once

#include <gst/gst.h>

#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWindow>
#include <qopenglvertexarrayobject.h>

namespace yams {

namespace details {
template <typename T> struct GstBufferUnrefer {
	void operator()(T *obj) const noexcept {
		if (obj != nullptr) {
			gst_buffer_unref(obj);
		}
	}
};

using GstBufferPtr = std::unique_ptr<GstBuffer, GstBufferUnrefer<GstBuffer>>;
}; // namespace details

class VideoWidget : public QOpenGLWindow, protected QOpenGLFunctions {
	Q_OBJECT
public:
	VideoWidget(QWindow *parent = nullptr);
	virtual ~VideoWidget();

public slots:

	void pushNewBuffer(void *buffer);

protected:
	void initializeGL() override;
	void paintGL() override;
	void resizeGL(int w, int h) override;

private:
	void setSize(int w, int h);

	QSize                    d_size = {0, 0};
	QOpenGLVertexArrayObject d_triangleVAO;
	QOpenGLBuffer            d_triangleVBO;
	QOpenGLShaderProgram     d_shader;
};
} // namespace yams
