#include "PlayerControl.hpp"
#include "ui_PlayerControl.h"

#include <QTime>


PlayerControl::PlayerControl(QWidget *parent)
	: QWidget(parent)
    , d_ui(new Ui::PlayerControl)
	, d_duration(0) {
	d_ui->setupUi(this);
	durationChanged(0);

	connect(d_ui->stopButton,SIGNAL(clicked()),
	        this,SIGNAL(stop()));
	connect(d_ui->prevButton,SIGNAL(clicked()),
	        this,SIGNAL(prev()));
	connect(d_ui->nextButton,SIGNAL(clicked()),
	        this,SIGNAL(next()));

}

PlayerControl::~PlayerControl() {
	delete d_ui;
}

QString PlayerControl::DurationToString(qint64 durationMS) {
	quint64 seconds = durationMS / 1000;
	quint64 milliseconds = durationMS - seconds*1000;
	quint64 hours = seconds / 3600;
	seconds -= hours * 3600;
	quint64 minutes = seconds / 60;
	seconds -= minutes * 60;

	QTime t(hours,minutes,seconds,milliseconds);

	return t.toString("hh:mm:ss.zzz");
}

void PlayerControl::durationChanged(qint64 durationMS) {
	d_duration = durationMS;
	d_ui->positionSlider->setMaximum(durationMS/1000);
	updatePositionInfo(qint64(d_ui->positionSlider->value()) * qint64(1000));
}

void PlayerControl::positionChanged(qint64 durationMS) {
	if ( ! d_ui->positionSlider->isSliderDown() ) {
		d_ui->positionSlider->setValue(durationMS/1000);
	}
	updatePositionInfo(durationMS);
}


void PlayerControl::on_positionSlider_sliderMoved(int value) {
	emit seek(qint64(value) * qint64(1000));
}


void PlayerControl::updatePositionInfo(qint64 durationMS) {
	if (durationMS > d_duration) {
		durationMS = d_duration;
	}
	d_ui->ellapsedLabel->setText(DurationToString(durationMS));
	d_ui->remainingLabel->setText("-"+DurationToString(d_duration-durationMS));
}


void PlayerControl::setState(QMediaPlayer::State state) {
	if ( state == d_state) {
		return;
	}

	d_state = state;
	switch(state) {
	case QMediaPlayer::StoppedState:
		d_ui->stopButton->setEnabled(false);
		d_ui->playButton->setIcon(QIcon::fromTheme("media-playback-start"));
		break;
	case QMediaPlayer::PlayingState:
		d_ui->stopButton->setEnabled(true);
		d_ui->playButton->setIcon(QIcon::fromTheme("media-playback-pause"));
		break;
	case QMediaPlayer::PausedState:
		d_ui->stopButton->setEnabled(true);
		d_ui->playButton->setIcon(QIcon::fromTheme("media-playback-start"));
		break;
	}

}

int PlayerControl::volume() const {
	qreal linearVolume =  QAudio::convertVolume(d_ui->volumeSlider->value() / qreal(100),
	                                            QAudio::LogarithmicVolumeScale,
	                                            QAudio::LinearVolumeScale);

	return qRound(linearVolume * 100);
}

void PlayerControl::setVolume(int volume) {
	qreal logarithmicVolume = QAudio::convertVolume(volume / qreal(100),
	                                                QAudio::LinearVolumeScale,
	                                                QAudio::LogarithmicVolumeScale);
	d_ui->volumeSlider->setValue(qRound(logarithmicVolume * 100));
}

qreal PlayerControl::playbackRate() const {
	return qMin(qMax(qreal(d_ui->speedSlider->value())/50.0 + 1.0,0.0),2.0);
}

void PlayerControl::setPlaybackRate(qreal rate) {
	int value = qMin(50,qMax(qRound((rate - 1.0) * 50.0),-50));
	d_ui->speedSlider->setValue(value);

}

qreal PlayerControl::opacity() const {
	return qMin(qMax(qreal(d_ui->opacitySlider->value())/100.0,0.0),1.0);
}


void PlayerControl::setOpacity(qreal opacity) {
	d_ui->opacitySlider->setValue(qMin(qMax(opacity,0.0),1.0)*100);
}

void PlayerControl::on_volumeSlider_valueChanged(int value) {
	emit changeVolume(volume());
}

void PlayerControl::on_opacitySlider_valueChanged(int value) {
	emit changeOpacity(opacity());
}

void PlayerControl::on_speedSlider_valueChanged(int value) {
	emit changeRate(playbackRate());
}

void PlayerControl::on_playButton_clicked() {
	switch(d_state) {
	case QMediaPlayer::StoppedState:
	case QMediaPlayer::PausedState:
		emit play();
		break;
	case QMediaPlayer::PlayingState:
		emit pause();
		break;
	}
}
