#include "MainWindow.hpp"
#include "ui_MainWindow.h"

#include "VideoWidget.hpp"
#include "PlaylistModel.hpp"

#include <QDebug>
#include <QColorDialog>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QFileInfo>
#include <QFileDialog>
#include <QStandardPaths>

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, d_ui(new Ui::MainWindow)
	, d_videoWidget(new VideoWidget())
	, d_mediaPlayer(new QMediaPlayer(this))
	, d_playlist(new QMediaPlaylist())
	, d_playlistModel(new PlaylistModel(this) ){
	d_mediaPlayer->setAudioRole(QAudio::VideoRole);
	d_mediaPlayer->setPlaylist(d_playlist);
	d_mediaPlayer->setVideoOutput(d_videoWidget);

	d_mediaPlayer->setObjectName(QStringLiteral("mediaPlayer"));
	d_playlist->setObjectName(QStringLiteral("playlist"));
	connect(d_videoWidget,SIGNAL(backgroundChanged(const QColor &)),
	        this,SLOT(onVideoWidgetBackgroundChanged(const QColor &)));




	d_ui->setupUi(this);
	d_videoWidget->show();

	d_playlistModel->setPlaylist(d_playlist);
	d_ui->listView->setModel(d_playlistModel);
	d_ui->listView->setCurrentIndex(d_playlistModel->index(d_playlist->currentIndex(), 0));



	d_ui->colorButton->setAutoFillBackground(true);
	d_ui->removeButton->setEnabled(false);

	onVideoWidgetBackgroundChanged(d_videoWidget->background());

	connect(d_mediaPlayer,SIGNAL(durationChanged(qint64)),
	        d_ui->playerControl,SLOT(durationChanged(qint64)));

	connect(d_mediaPlayer,SIGNAL(positionChanged(qint64)),
	        d_ui->playerControl,SLOT(positionChanged(qint64)));

	connect(d_ui->playerControl, SIGNAL(play()),
	        d_mediaPlayer, SLOT(play()));
	connect(d_ui->playerControl, SIGNAL(pause()),
	        d_mediaPlayer, SLOT(pause()));
	connect(d_ui->playerControl, SIGNAL(stop()),
	        d_mediaPlayer, SLOT(stop()));
	connect(d_ui->playerControl, SIGNAL(next()),
	        d_playlist, SLOT(next()));
	//	connect(d_ui->playerControl, SIGNAL(previous),
	//        this, &Player::previousClicked);
	connect(d_ui->playerControl, SIGNAL(changeVolume(int)),
	        d_mediaPlayer, SLOT(setVolume(int)));
	connect(d_ui->playerControl, SIGNAL(changeRate(qreal)),
	        d_mediaPlayer, SLOT(setPlaybackRate(qreal)));
	connect(d_ui->playerControl, SIGNAL(stop()),
	        d_videoWidget, SLOT(update()));
	connect(d_ui->playerControl, SIGNAL(changeOpacity(qreal)),
	        d_videoWidget, SLOT(setOpacity(qreal)));

	d_ui->playerControl->setState(d_mediaPlayer->state());
	d_ui->playerControl->setVolume(d_mediaPlayer->volume());
	d_ui->playerControl->setPlaybackRate(d_mediaPlayer->playbackRate());
	d_ui->playerControl->setOpacity(d_videoWidget->opacity());
	connect(d_mediaPlayer, SIGNAL(stateChanged(QMediaPlayer::State)),
	        d_ui->playerControl, SLOT(setState(QMediaPlayer::State)));

	connect(d_mediaPlayer, SIGNAL(volumeChanged(int)),
	        d_ui->playerControl, SLOT(setVolume(int)));

	connect(d_mediaPlayer, SIGNAL(playbackRateChanged(qreal)),
	        d_ui->playerControl, SLOT(setPlaybackRate(qreal)));

	connect(d_videoWidget, SIGNAL(opacityChanged(qreal)),
	        d_ui->playerControl, SLOT(setOpacity(qreal)));

}

MainWindow::~MainWindow() {
	d_videoWidget->close();
	delete d_videoWidget;
	delete d_ui;
}


void MainWindow::on_colorButton_clicked() {
	d_videoWidget->setBackground(QColorDialog::getColor(d_videoWidget->background(),this));
}


void MainWindow::onVideoWidgetBackgroundChanged(const QColor & color) {
	d_ui->colorButton->setText("");
	QPixmap pixmap(d_ui->colorButton->iconSize());
	pixmap.fill(color);
	d_ui->colorButton->setIcon(QIcon(pixmap));
}

void MainWindow::on_mediaPlayer_metaDataChanged() {
}

void MainWindow::on_mediaPlayer_mediaStatusChanged(QMediaPlayer::MediaStatus status) {
#ifndef QT_NO_CURSOR
	if ( status == QMediaPlayer::LoadingMedia ||
	     status == QMediaPlayer::BufferingMedia ||
	     status == QMediaPlayer::StalledMedia ) {
		setCursor(QCursor(Qt::BusyCursor));
	} else {
		unsetCursor();
	}
#endif

	switch(status) {
	case QMediaPlayer::UnknownMediaStatus:
	case QMediaPlayer::NoMedia:
	case QMediaPlayer::LoadedMedia:
		setStatusInfo("");
		break;
	case QMediaPlayer::LoadingMedia:
		setStatusInfo(tr("Loading..."));
		break;
	case QMediaPlayer::BufferingMedia:
	case QMediaPlayer::BufferedMedia:
        setStatusInfo(tr("Buffering %1%").arg(d_mediaPlayer->bufferStatus()));
        break;
	case QMediaPlayer::StalledMedia:
		setStatusInfo(tr("Stalled %1%").arg(d_mediaPlayer->bufferStatus()));
		break;
	case QMediaPlayer::EndOfMedia:
		QApplication::alert(this);
        break;
	case QMediaPlayer::InvalidMedia:
		on_mediaPlayer_error();
		break;


	}
}

void MainWindow::on_mediaPlayer_error() {
	setStatusInfo(d_mediaPlayer->errorString());
}

void MainWindow::setStatusInfo(const QString & info) {
	d_ui->statusbar->showMessage(info);
}


void MainWindow::on_playerControl_seek(qint64 durationMS) {
	d_mediaPlayer->setPosition(durationMS);
}

bool MainWindow::isM3uPlaylist(const QUrl &url) {
	if (!url.isLocalFile()) {
        return false;
	}
	const QFileInfo fileInfo(url.toLocalFile());
	return fileInfo.exists() && !fileInfo.suffix().compare(QLatin1String("m3u"), Qt::CaseInsensitive);
}


void MainWindow::on_addButton_clicked() {
	QFileDialog fileDialog(this);
	fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
	fileDialog.setWindowTitle(tr("Open Files"));
	QStringList supportedMimeTypes = d_mediaPlayer->supportedMimeTypes();
	if (!supportedMimeTypes.isEmpty()) {
		supportedMimeTypes.append("audio/x-m3u"); // MP3 playlists
		fileDialog.setMimeTypeFilters(supportedMimeTypes);
	}
	fileDialog.setDirectory(QStandardPaths::standardLocations(QStandardPaths::MoviesLocation).value(0, QDir::homePath()));
	if (fileDialog.exec() == QDialog::Accepted) {
		addToPlaylist(fileDialog.selectedUrls());
	}
}

void MainWindow::on_removeButton_clicked() {

}

void MainWindow::addToPlaylist(const QList<QUrl> &urls) {
	for ( const auto & url : urls) {
		if ( isM3uPlaylist(url) ){
			d_playlist->load(url);
		} else {
			d_playlist->addMedia(url);
		}
	}
}

void MainWindow::on_listView_activated(const QModelIndex & index) {
	if (!index.isValid()) {
		return;
	}
	d_playlist->setCurrentIndex(index.row());
	d_mediaPlayer->play();
}
