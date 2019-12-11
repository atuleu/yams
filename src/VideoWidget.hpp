#pragma once

#include <QVideoWidget>

class VideoWidget : public QVideoWidget {
	Q_OBJECT

	Q_PROPERTY(QColor background
	           READ background
	           WRITE setBackground
	           NOTIFY backgroundChanged);

	Q_PROPERTY(qreal opacity
	           READ opacity
	           WRITE setOpacity
	           NOTIFY opacityChanged);



public:
	explicit VideoWidget(QWidget * parent = 0);
	~VideoWidget();

	QColor background() const;

	qreal opacity() const;

public slots:
	void setBackground(const QColor & color);
	void setOpacity(qreal);

signals:
	void backgroundChanged(const QColor & color);
	void opacityChanged(qreal);
protected:
    void keyPressEvent(QKeyEvent *event) override;
	void mouseDoubleClickEvent(QMouseEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
private:

	QColor d_background;
};
