#include "MediaPlayer.hpp"


MediaPlayer::MediaPlayer(QWidget * parent)
	: QWidget(parent) {
	setAutoFillBackground(true);
	setPalette(Qt::black);
}


MediaPlayer::~MediaPlayer() {
}
