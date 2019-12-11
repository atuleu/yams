#pragma once

#include <QMainWindow>
#include <QMediaPlayer>
namespace Ui {
class MainWindow;
}

class VideoWidget;
class PlaylistModel;
class QMediaPlayer;

class MainWindow : public QMainWindow {
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

	static bool isM3uPlaylist(const QUrl & url);
public slots:
	void on_listView_activated(const QModelIndex &);
	void on_colorButton_clicked();
	void onVideoWidgetBackgroundChanged(const QColor & color);
	void on_mediaPlayer_metaDataChanged();
	void on_mediaPlayer_mediaStatusChanged(QMediaPlayer::MediaStatus status);
	void on_mediaPlayer_error();

	void on_playerControl_seek(qint64 durationMS);

	void on_addButton_clicked();
	void on_removeButton_clicked();

protected:

	void setStatusInfo(const QString & info);
	void addToPlaylist(const QList<QUrl> & urls);

private:
	Ui::MainWindow *d_ui;

	VideoWidget * d_videoWidget;
	QMediaPlayer * d_mediaPlayer;
	QMediaPlaylist * d_playlist;
	PlaylistModel  * d_playlistModel;
};
