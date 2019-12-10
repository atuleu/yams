#pragma once

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class VideoWidget;

class MainWindow : public QMainWindow {
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

public slots:

	void on_colorButton_clicked();
	void onVideoWidgetBackgroundChanged(const QColor & color);

private:
	Ui::MainWindow *d_ui;

	VideoWidget * d_videoWidget;
};
