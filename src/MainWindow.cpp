#include "MainWindow.hpp"
#include "ui_MainWindow.h"

#include "VideoWidget.hpp"

#include <QDebug>
#include <QColorDialog>

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent),
	  d_ui(new Ui::MainWindow) {
	d_ui->setupUi(this);
	d_videoWidget = new VideoWidget();
	d_videoWidget->show();

	d_ui->colorButton->setAutoFillBackground(true);

	connect(d_videoWidget,SIGNAL(backgroundChanged(const QColor &)),
	        this,SLOT(onVideoWidgetBackgroundChanged(const QColor &)));

	onVideoWidgetBackgroundChanged(d_videoWidget->background());

}

MainWindow::~MainWindow() {
	delete d_videoWidget;
	delete d_ui;
}


void MainWindow::on_colorButton_clicked() {
	d_videoWidget->setBackground(QColorDialog::getColor(d_videoWidget->background(),this));
}


void MainWindow::onVideoWidgetBackgroundChanged(const QColor & color) {
	d_ui->colorButton->setText("");
	QPixmap pixmap(d_ui->colorButton->iconSize());
	pixmap.fill(color);
	d_ui->colorButton->setIcon(QIcon(pixmap));

}
