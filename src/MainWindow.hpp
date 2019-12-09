#pragma once

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MediaPlayer;

class MainWindow : public QMainWindow {
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

private:
	Ui::MainWindow *d_ui;

	MediaPlayer * d_mediaPlayer;
};
