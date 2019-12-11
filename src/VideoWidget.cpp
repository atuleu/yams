#include "VideoWidget.hpp"


#include <QKeyEvent>
#include <QDebug>
#include <QCloseEvent>


VideoWidget::VideoWidget(QWidget * parent)
	: QVideoWidget(parent)
	, d_background(Qt::black)
	, d_acceptClose(false){
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
void VideoWidget::setAcceptClose(bool accept) {
	d_acceptClose = accept;
}
void VideoWidget::closeEvent(QCloseEvent *event) {
	if (d_acceptClose) {
		event->accept();
	} else {
		event->ignore();
	}
}
