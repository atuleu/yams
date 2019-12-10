#include "MainWindow.hpp"
#include "ui_MainWindow.h"

#include "VideoWidget.hpp"

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent),
	  d_ui(new Ui::MainWindow) {
	d_ui->setupUi(this);
	d_videoWidget = new VideoWidget();
	d_videoWidget->show();
}

MainWindow::~MainWindow() {
	delete d_videoWidget;
	delete d_ui;
}
