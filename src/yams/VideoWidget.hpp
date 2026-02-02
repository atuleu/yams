#pragma once

#include <gst/gst.h>

#include <QWidget>

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

class VideoWidget : public QWidget {
	Q_OBJECT
public:
	VideoWidget(QWidget *parent = nullptr)
	    : QWidget(parent) {}

	virtual ~VideoWidget() = default;

public slots:

	void pushNewBuffer(void *buffer);

protected:
	void paintEvent(QPaintEvent *event) override;

private:
	details::GstBufferPtr d_buffer = nullptr;
};
} // namespace yams
