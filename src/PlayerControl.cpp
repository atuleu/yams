#include "PlayerControl.hpp"
#include "ui_PlayerControl.h"

#include <QTime>


PlayerControl::PlayerControl(QWidget *parent)
	: QWidget(parent)
    , d_ui(new Ui::PlayerControl)
	, d_duration(0) {
	d_ui->setupUi(this);
	durationChanged(0);
}

PlayerControl::~PlayerControl() {
	delete d_ui;
}

QString PlayerControl::DurationToString(qint64 durationMS) {

	quint64 seconds = durationMS / 1000;
	quint64 milliseconds = durationMS - seconds;
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
	updatePositionInfo(quint64(d_ui->positionSlider->value()) * quint64(1000));
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
