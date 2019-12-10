#include "PlaylistModel.hpp"

#include <QFileInfo>
#include <QUrl>
#include <QMediaPlaylist>

PlaylistModel::PlaylistModel(QObject *parent)
	: QAbstractItemModel(parent) {
}

PlaylistModel::~PlaylistModel() {
}

int PlaylistModel::rowCount(const QModelIndex &parent) const {
	return d_playlist && !parent.isValid() ? d_playlist->mediaCount() : 0;
}

int PlaylistModel::columnCount(const QModelIndex &parent) const {
	return !parent.isValid() ? ColumnCount : 0;
}

QModelIndex PlaylistModel::index(int row, int column, const QModelIndex &parent) const {
	return d_playlist
		&& !parent.isValid()
		&& row >= 0
		&& row < d_playlist->mediaCount()
		&& column >= 0
		&& column < ColumnCount
		            ? createIndex(row, column)
		            : QModelIndex();
}

QModelIndex PlaylistModel::parent(const QModelIndex &child) const {
	Q_UNUSED(child);

	return QModelIndex();
}

QVariant PlaylistModel::data(const QModelIndex &index, int role) const {
	if (index.isValid() && role == Qt::DisplayRole) {
		QVariant value = d_data[index];
		if (!value.isValid() && index.column() == Title) {
			QUrl location = d_playlist->media(index.row()).canonicalUrl();
			return QFileInfo(location.path()).fileName();
        }

		return value;
	}
    return QVariant();
}

QMediaPlaylist *PlaylistModel::playlist() const {
	return d_playlist.data();
}

void PlaylistModel::setPlaylist(QMediaPlaylist *playlist) {
	if (d_playlist) {
		disconnect(d_playlist.data(), &QMediaPlaylist::mediaAboutToBeInserted, this, &PlaylistModel::beginInsertItems);
        disconnect(d_playlist.data(), &QMediaPlaylist::mediaInserted, this, &PlaylistModel::endInsertItems);
        disconnect(d_playlist.data(), &QMediaPlaylist::mediaAboutToBeRemoved, this, &PlaylistModel::beginRemoveItems);
        disconnect(d_playlist.data(), &QMediaPlaylist::mediaRemoved, this, &PlaylistModel::endRemoveItems);
        disconnect(d_playlist.data(), &QMediaPlaylist::mediaChanged, this, &PlaylistModel::changeItems);
    }

	beginResetModel();
	d_playlist.reset(playlist);

    if (d_playlist) {
	    connect(d_playlist.data(), &QMediaPlaylist::mediaAboutToBeInserted, this, &PlaylistModel::beginInsertItems);
        connect(d_playlist.data(), &QMediaPlaylist::mediaInserted, this, &PlaylistModel::endInsertItems);
        connect(d_playlist.data(), &QMediaPlaylist::mediaAboutToBeRemoved, this, &PlaylistModel::beginRemoveItems);
        connect(d_playlist.data(), &QMediaPlaylist::mediaRemoved, this, &PlaylistModel::endRemoveItems);
        connect(d_playlist.data(), &QMediaPlaylist::mediaChanged, this, &PlaylistModel::changeItems);
    }

    endResetModel();
}

bool PlaylistModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    Q_UNUSED(role);
    d_data[index] = value;
    emit dataChanged(index, index);
    return true;
}

void PlaylistModel::beginInsertItems(int start, int end) {
    d_data.clear();
    beginInsertRows(QModelIndex(), start, end);
}

void PlaylistModel::endInsertItems() {
    endInsertRows();
}

void PlaylistModel::beginRemoveItems(int start, int end) {
    d_data.clear();
    beginRemoveRows(QModelIndex(), start, end);
}

void PlaylistModel::endRemoveItems() {
    endInsertRows();
}

void PlaylistModel::changeItems(int start, int end) {
	d_data.clear();
    emit dataChanged(index(start,0), index(end,ColumnCount));
}
