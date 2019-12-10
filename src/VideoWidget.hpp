#pragma once

#include <QVideoWidget>

class VideoWidget : public QVideoWidget {
	Q_OBJECT

	Q_PROPERTY(QColor background
	           READ background
	           WRITE setBackground
	           NOTIFY backgroundChanged);



public:
	explicit VideoWidget(QWidget * parent = 0);
	~VideoWidget();

	QColor background() const;

public slots:
	void setBackground(const QColor & color);

signals:
	void backgroundChanged(const QColor & color);

protected:
    void keyPressEvent(QKeyEvent *event) override;
	void mouseDoubleClickEvent(QMouseEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
private:

	QColor d_background;
};
