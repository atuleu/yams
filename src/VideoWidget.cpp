#include "VideoWidget.hpp"


#include <QKeyEvent>
#include <QDebug>


VideoWidget::VideoWidget(QWidget * parent)
	: QVideoWidget(parent)
	, d_background(Qt::black){
	setAutoFillBackground(true);
	setPalette(d_background);
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

QColor VideoWidget::background() const {
	return d_background;
}

void VideoWidget::setBackground(const QColor & color) {
	bool emitSignal = color != d_background;
	d_background = color;
	if ( emitSignal == true ) {
		setPalette(d_background);
		emit backgroundChanged(d_background);
	}
}


qreal VideoWidget::opacity() const {
	return 1.0;
}

void VideoWidget::setOpacity(qreal opacity) {
	qInfo() << "Set opacity" << opacity;
}
