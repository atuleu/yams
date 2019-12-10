#pragma once

#include <QVideoWidget>

class VideoWidget : public QVideoWidget {
	Q_OBJECT

public:
	explicit VideoWidget(QWidget * parent = 0);
	~VideoWidget();

protected:
    void keyPressEvent(QKeyEvent *event) override;
	void mouseDoubleClickEvent(QMouseEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
};
