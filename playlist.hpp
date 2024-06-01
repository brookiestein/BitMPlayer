#ifndef PLAYLIST_HPP
#define PLAYLIST_HPP

#include <map>
#include <QObject>
#include <QSettings>
#include <QWidget>

class Playlist : public QObject
{
    Q_OBJECT
    QWidget *m_parent;
    QString m_configFile;
    QSettings *m_settings;
public:
    explicit Playlist(QWidget *parent = nullptr);
    std::map<QString, QString> openPlayList();
    int removePlaylists();
    void savePlayList(QString playlistName, std::map<QString, QString> songs);
};

#endif // PLAYLIST_HPP
