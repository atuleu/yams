#include "VideoWidget.hpp"

#include <QGraphicsVideoItem>
#include <QKeyEvent>
#include <QDebug>
#include <QCloseEvent>
#include <QGraphicsOpacityEffect>
#include <QVBoxLayout>

VideoWidget::VideoWidget(QWidget * parent)
	: QGraphicsView(new QGraphicsScene(), parent)
	, d_background(Qt::black)
	, d_acceptClose(false)
	, d_videoWidget(new QGraphicsVideoItem()) {
	setStyleSheet("border: 0px");

	setBackgroundBrush(d_background);
	scene()->addItem(d_videoWidget);
	d_videoWidget->setOpacity(1.0);
}


VideoWidget::~VideoWidget() {
}

void VideoWidget::keyPressEvent(QKeyEvent *event) {
#ifndef NDEBUG
	if (event->key() == Qt::Key_Escape && isFullScreen()) {
		showNormal();
	}
#endif
	event->accept();
}

void VideoWidget::mouseDoubleClickEvent(QMouseEvent *event) {
#ifndef NDEBUG
	if(isFullScreen()) {
		showNormal();
	} else {
		showFullScreen();
	}
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
		setBackgroundBrush(d_background);
		emit backgroundChanged(d_background);
	}
}


qreal VideoWidget::opacity() const {
	return d_videoWidget->opacity();
}

void VideoWidget::setOpacity(qreal opacity) {
	d_videoWidget->setOpacity(opacity);
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

QGraphicsVideoItem * VideoWidget::QVW() {
	return d_videoWidget;
}
