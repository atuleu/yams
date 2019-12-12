#include "VideoWidget.hpp"

#include <QGraphicsVideoItem>
#include <QKeyEvent>
#include <QDebug>
#include <QCloseEvent>
#include <QGraphicsOpacityEffect>
#include <QVBoxLayout>
#include <QSettings>
#include <QScreen>

VideoWidget::VideoWidget(QWidget * parent)
	: QGraphicsView(new QGraphicsScene(), parent)
	, d_background(Qt::black)
	, d_acceptClose(false)
	, d_videoWidget(new QGraphicsVideoItem()) {
	setStyleSheet("border: 0px");

	setBackgroundBrush(d_background);
	scene()->addItem(d_videoWidget);
	d_videoWidget->setOpacity(1.0);

	// connect(d_videoWidget,SIGNAL(nativeSizeChanged(const QSizeF &)),
	//         this,SLOT(updateSize()));
	d_videoWidget->setSize(size());
	setSceneRect(QRectF(QPointF(0,0),size()));

	restoreWindowGeometry();
}


VideoWidget::~VideoWidget() {
}

void VideoWidget::keyPressEvent(QKeyEvent *event) {
#ifndef NDEBUG
	if (event->key() == Qt::Key_Escape && isFullScreen()) {
		setFullScreen(false,NULL);
	}
#endif
	event->accept();
}

void VideoWidget::mouseDoubleClickEvent(QMouseEvent *event) {
#ifndef NDEBUG
	setFullScreen(!isFullScreen(),NULL);
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
		saveWindowGeometry();
		event->accept();
	} else {
		event->ignore();
	}
}

QGraphicsVideoItem * VideoWidget::QVW() {
	return d_videoWidget;
}


void VideoWidget::resizeEvent(QResizeEvent * event) {
	d_videoWidget->setSize(size());
	setSceneRect(QRectF(QPointF(0,0),size()));
	QGraphicsView::resizeEvent(event);
}


void VideoWidget::setFullScreen(bool fullscreen,QScreen * screen) {
	if ( isFullScreen() == fullscreen ) {
		//nothing to do
		return;
	}

	if ( fullscreen == true ) {
		saveWindowGeometry();
		if ( screen != NULL ) {
			auto geom = screen->geometry();
			move(QPoint(geom.x(),geom.y()));
		}
		showFullScreen();
	} else {
		showNormal();
		restoreWindowGeometry();
	}

}

void VideoWidget::saveWindowGeometry() {
	QSettings settings;
	settings.setValue("videoOutput/geometry", saveGeometry());
}

void VideoWidget::restoreWindowGeometry() {
	QSettings settings;
	restoreGeometry(settings.value("videoOutput/geometry").toByteArray());
}
