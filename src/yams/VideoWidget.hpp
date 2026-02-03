#pragma once

#include <gst/gst.h>

#include <QOpenGLWindow>
#include <qopenglwindow.h>

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

class VideoWidget : public QOpenGLWindow {
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
};
} // namespace yams
