#pragma once

#include <QWidget>

class MediaPlayer : public QWidget {
	Q_OBJECT
public:
	explicit MediaPlayer(QWidget * parent = 0);
	~MediaPlayer();

};
