#pragma once

#include <QWidget>

namespace Ui {
class PlayerControl;
}

class PlayerControl : public QWidget {
	Q_OBJECT
public:
	explicit PlayerControl(QWidget *parent = nullptr);
	~PlayerControl();

	static QString DurationToString(qint64 durationMS);

public slots:
	void durationChanged(qint64 durationMS);
	void positionChanged(qint64 durationMS);

protected slots:
	void on_positionSlider_sliderMoved(int value);

signals:
	void seek(qint64 durationMS);

protected:
	void updatePositionInfo(qint64 durationMS);

private:
	Ui::PlayerControl * d_ui;

	quint64 d_duration;
};
