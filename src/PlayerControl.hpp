#pragma once

#include <QWidget>
#include <QMediaPlayer>

namespace Ui {
class PlayerControl;
}

class PlayerControl : public QWidget {
	Q_OBJECT
public:
	explicit PlayerControl(QWidget *parent = nullptr);
	~PlayerControl();

	static QString DurationToString(qint64 durationMS);

	int volume() const;

	qreal opacity() const;

	qreal playbackRate() const;

public slots:
	void durationChanged(qint64 durationMS);
	void positionChanged(qint64 durationMS);
	void setState(QMediaPlayer::State state);
	void setVolume(int);
	void setPlaybackRate(qreal);
	void setOpacity(qreal);



protected slots:
	void on_positionSlider_sliderMoved(int value);
	void on_volumeSlider_valueChanged(int value);
	void on_opacitySlider_valueChanged(int value);
	void on_speedSlider_valueChanged(int value);
	void on_playButton_clicked();

signals:
	void seek(qint64 durationMS);
	void play();
	void pause();
	void stop();
	void next();
	void prev();
	void changeVolume(int);
	void changeRate(qreal);
	void changeOpacity(qreal);

protected:
	void updatePositionInfo(qint64 durationMS);

private:
	Ui::PlayerControl * d_ui;

	quint64 d_duration;
	QMediaPlayer::State d_state;
};
