#include "VideoWidget.hpp"


#include <QKeyEvent>

VideoWidget::VideoWidget(QWidget * parent)
	: QVideoWidget(parent) {
	setAutoFillBackground(true);
	setPalette(Qt::black);
}


VideoWidget::~VideoWidget() {
}


void VideoWidget::keyPressEvent(QKeyEvent *event) {
#ifndef NDEBUG
	if (event->key() == Qt::Key_Escape && isFullScreen()) {
		setFullScreen(false);
	}
#endif
	event->accept();
}

void VideoWidget::mouseDoubleClickEvent(QMouseEvent *event) {
#ifndef NDEBUG
    setFullScreen(!isFullScreen());
#endif
    event->accept();
}

void VideoWidget::mousePressEvent(QMouseEvent *event) {
	event->accept();
}
