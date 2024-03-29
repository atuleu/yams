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
#include <QCloseEvent>
#include <QSettings>
#include <QScreen>

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, d_ui(new Ui::MainWindow)
	, d_videoWidget(new VideoWidget())
	, d_mediaPlayer(new QMediaPlayer(this))
	, d_playlist(new QMediaPlaylist())
	, d_playlistModel(new PlaylistModel(this) ){
	d_mediaPlayer->setAudioRole(QAudio::VideoRole);
	d_mediaPlayer->setPlaylist(d_playlist);
	d_mediaPlayer->setVideoOutput(d_videoWidget->QVW());

	d_mediaPlayer->setObjectName(QStringLiteral("mediaPlayer"));
	d_playlist->setObjectName(QStringLiteral("playlist"));
	connect(d_videoWidget,SIGNAL(backgroundChanged(const QColor &)),
	        this,SLOT(onVideoWidgetBackgroundChanged(const QColor &)));

	connect(d_playlist,SIGNAL(mediaInserted(int,int)),
	        this,SLOT(onPlaylistMediaInserted(int,int)));
	connect(d_playlist,SIGNAL(mediaRemoved(int,int)),
	        this,SLOT(onPlaylistMediaRemoved(int,int)));



	d_ui->setupUi(this);
	d_ui->playbackBox->insertItem(0,tr("Current item once"),QMediaPlaylist::CurrentItemOnce);
	d_ui->playbackBox->insertItem(1,tr("Current item in loop"),QMediaPlaylist::CurrentItemInLoop);
	d_ui->playbackBox->insertItem(2,tr("Sequence"),QMediaPlaylist::Sequential);
	d_ui->playbackBox->insertItem(3,tr("Loop"),QMediaPlaylist::Loop);
	d_ui->playbackBox->insertItem(4,tr("Random"),QMediaPlaylist::Random);

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


	connect(d_ui->listView->selectionModel(),
	        SIGNAL(selectionChanged(const QItemSelection &,const QItemSelection &)),
	        this,
	        SLOT(selectionChanged(const QItemSelection &,const QItemSelection &)));


	onScreenChange(NULL);
	connect(QGuiApplication::instance(),SIGNAL(screenAdded(QScreen*)),
	        this,SLOT(onScreenChange(QScreen*)));

	connect(QGuiApplication::instance(),SIGNAL(screenRemoved(QScreen*)),
	        this,SLOT(onScreenChange(QScreen*)));

	QSettings settings;

	restoreGeometry(settings.value("mainWindow/geometry").toByteArray());
	restoreState(settings.value("mainWindow/state").toByteArray());
	restoreCompositionState();
}

MainWindow::~MainWindow() {
	delete d_videoWidget;
	delete d_ui;
}

void MainWindow::closeEvent(QCloseEvent * event) {
	if ( d_ui->liveButton->isChecked() ) {
		event->ignore();
		return;
	}
	saveCompositionState();
	QSettings settings;
	settings.setValue("mainWindow/geometry",saveGeometry());
	settings.setValue("mainWindow/state",saveState());

	d_videoWidget->setAcceptClose(true);
	d_videoWidget->close();

	QMainWindow::closeEvent(event);
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
	auto selected = d_ui->listView->selectionModel()->selectedRows();
	for(const auto & r : selected) {
		d_playlist->removeMedia(r.row());
	}
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
}


void MainWindow::selectionChanged(const QItemSelection & selected,
                                  const QItemSelection & deselected) {
	if (selected.isEmpty()) {
		d_ui->removeButton->setEnabled(false);
	} else {
		d_ui->removeButton->setEnabled(true);
	}

}

void MainWindow::onPlaylistMediaInserted(int,int) {
	d_ui->clearButton->setEnabled(!d_playlist->isEmpty());
}

void MainWindow::onPlaylistMediaRemoved(int,int) {
	d_ui->clearButton->setEnabled(!d_playlist->isEmpty());
}

void MainWindow::on_clearButton_clicked() {
	d_playlist->clear();
}


void MainWindow::onScreenChange(QScreen *) {
	auto selected = d_ui->screenBox->currentData().toString();
	qInfo() << selected;
	d_ui->screenBox->clear();
	bool noSelection = true;
	int index = 0;
	for(const auto & s : QGuiApplication::screens()) {
		QString name = s->name() + "-" + s->manufacturer() + "-" + s->model();
		d_ui->screenBox->insertItem(index,name,name);
		if ( name == selected) {
			noSelection = false;
			d_ui->screenBox->setCurrentIndex(index);
		}
		++index;
	}
	if ( noSelection == true ) {
		d_ui->screenBox->setCurrentIndex(-1);
	}
}

void MainWindow::on_screenBox_currentIndexChanged(int index) {
	if (index < 0 ) {
		d_ui->liveButton->setChecked(false);
		d_ui->liveButton->setEnabled(false);
		return;
	}
	d_ui->liveButton->setEnabled(true);
}


void MainWindow::on_liveButton_toggled(bool value)  {
	auto screen = QGuiApplication::screens()[d_ui->screenBox->currentIndex()];
	d_videoWidget->setFullScreen(value,screen);
}


void MainWindow::saveCompositionState() {
	QSettings settings;

	settings.setValue("composition/backgroundColor",d_videoWidget->background().name());
	settings.setValue("composition/screenName",d_ui->screenBox->currentData().toString());
	if(d_ui->screenBox->currentIndex() < 0 ) {
		settings.setValue("composition/isLive",false);
	} else {
		settings.setValue("composition/isLive",d_ui->liveButton->isChecked());
	}
	settings.setValue("composition/opacity",d_ui->playerControl->opacity());
	settings.setValue("composition/rate",d_ui->playerControl->playbackRate());
	settings.setValue("composition/volume",d_ui->playerControl->volume());
	QStringList urls;
	for(int i = 0; i < d_playlist->mediaCount(); ++i){
		urls.push_back(d_playlist->media(i).canonicalUrl().toString());
	}
	settings.setValue("composition/playlist",urls);
	settings.setValue("composition/playbackMode",d_ui->playbackBox->currentData().toInt());
}


void MainWindow::restoreCompositionState() {
	QSettings settings;

	QColor background(settings.value("composition/backgroundColor","#000000").toString());
	d_videoWidget->setBackground(background);
	auto screen =  settings.value("composition/screenName").toString();
	for ( int i = 0; i < d_ui->screenBox->count(); ++i) {
		if ( d_ui->screenBox->itemData(i).toString() == screen ) {
			d_ui->screenBox->setCurrentIndex(i);
			break;
		}
	}
	if ( d_ui->screenBox->currentIndex() < 0 ) {
		d_ui->liveButton->setChecked(false);
	} else {
		d_ui->liveButton->setChecked(settings.value("composition/isLive",false).toBool());
	}
	d_ui->playerControl->setOpacity(settings.value("composition/opacity",1.0).toDouble());
	//d_ui->playerControl->setPlaybackRate(settings.value("composition/rate",1.0).toDouble());
	d_ui->playerControl->setPlaybackRate(1.0);
	d_ui->playerControl->setVolume(settings.value("composition/volume",100).toInt());

	for(const auto & url: settings.value("composition/playlist").toStringList() ) {
		addToPlaylist({url});
	}
	auto mode = (QMediaPlaylist::PlaybackMode)settings.value("composition/playbackMode",QMediaPlaylist::Sequential).toInt();
	for(int i = 0; i < d_ui->playbackBox->count(); ++i) {
		auto bmode = (QMediaPlaylist::PlaybackMode)d_ui->playbackBox->itemData(i).toInt();
		if ( mode == bmode ) {
			d_ui->playbackBox->setCurrentIndex(i);
			break;
		}
	}
}

void MainWindow::on_playbackBox_currentIndexChanged(int index) {
	if ( index < 0 ){
		return;
	}

	d_playlist->setPlaybackMode((QMediaPlaylist::PlaybackMode)d_ui->playbackBox->currentData().toInt());
}
