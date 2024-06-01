#include <QDir>
#include <QEventLoop>
#include <QStandardPaths>

#include "playlist.hpp"
#include "playlistselector.hpp"

Playlist::Playlist(QWidget *parent)
    : QObject(parent)
    , m_parent(parent)
{
    m_configFile = QString("%1%2%3%4%5")
                       .arg(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation),
                            QDir::separator(), PROGRAM_NAME, QDir::separator(), PROGRAM_NAME".ini");
    m_settings = new QSettings(m_configFile, QSettings::IniFormat, this);
}

std::map<QString, QString> Playlist::openPlayList()
{
    std::map<QString, QString> playlist;

    auto playlistNames = m_settings->childGroups();
    QEventLoop loop;
    PlaylistSelector selector(playlistNames);
    connect(&selector, &PlaylistSelector::closed, &loop, &QEventLoop::quit);
    selector.show();
    loop.exec();

    auto playlistName = selector.getSelection();
    if (playlistName.isEmpty()) {
        return {};
    }

    m_settings->beginGroup(playlistName[0]); /* PlaylistSelector will return a QStringList with just one QString */
    for (auto key : m_settings->childKeys()) {
        playlist[key] = m_settings->value(key).toString();
    }
    m_settings->endGroup();

    return playlist;
}

int Playlist::removePlaylists()
{
    int removed {};
    auto playlistNames = m_settings->childGroups();
    QEventLoop loop;
    PlaylistSelector selector(playlistNames, QAbstractItemView::MultiSelection);
    connect(&selector, &PlaylistSelector::closed, &loop, &QEventLoop::quit);
    selector.show();
    loop.exec();

    auto selection = selector.getSelection();
    if (selection.isEmpty()) {
        goto exit;
    }

    for (auto group : selection) {
        m_settings->beginGroup(group);
        m_settings->remove("");
        m_settings->endGroup();
        ++removed;
    }

exit:
    return removed;
}

void Playlist::savePlayList(QString playlistName, std::map<QString, QString> songs)
{
    m_settings->beginGroup(playlistName);
    for (const auto [songName, filePath] : songs) {
        m_settings->setValue(songName, filePath);
    }
    m_settings->endGroup();
}
