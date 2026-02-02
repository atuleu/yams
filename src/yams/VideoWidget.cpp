#include "VideoWidget.hpp"

#include <QPainter>
#include <gst/gstmemory.h>

namespace yams {
void VideoWidget::pushNewBuffer(void *buffer) {
	d_buffer.reset(reinterpret_cast<GstBuffer *>(buffer));
	update();
}

void VideoWidget::paintEvent(QPaintEvent *event) {
	Q_UNUSED(event);
	QPainter painter(this);
	painter.fillRect(rect(), Qt::black);

	if (d_buffer == nullptr) {
		return;
	}
	GstMapInfo map;
	if (gst_buffer_map(d_buffer.get(), &map, GST_MAP_READ) == false) {
		return;
	}

	QImage img(map.data, 1920, 1080, QImage::Format_BGR888);
	painter.drawImage(rect(), img);
	gst_buffer_unmap(d_buffer.get(), &map);
}

} // namespace yams
