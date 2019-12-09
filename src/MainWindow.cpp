#include "MainWindow.hpp"
#include "ui_MainWindow.h"

#include "MediaPlayer.hpp"

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent),
	  d_ui(new Ui::MainWindow) {
	d_ui->setupUi(this);
	d_mediaPlayer = new MediaPlayer();
	d_mediaPlayer->show();
}

MainWindow::~MainWindow() {
	delete d_mediaPlayer;
	delete d_ui;
}
