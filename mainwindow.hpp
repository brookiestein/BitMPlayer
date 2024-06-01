#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <map>
#include <QColor>
#include <QMainWindow>
#include <QTimer>

#include <raylib.h>

#include "playlist.hpp"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
    Ui::MainWindow *ui;
    QTimer m_musicTimer;
    QTimer m_sliderTimer;
    QTimer m_statusTimer;
    Music m_music;
    QStringList m_filenames;
    std::map<QString, QString> m_splittedSongs;
    QString m_musicPlaying;
    unsigned int m_musicCount;
    bool m_firstTime;
    float m_streamLength;
    Playlist *m_playlist;

    void resetControllers(bool resetLength = true, bool resetPlayingEdit = true);
    void setStatusText(QString text, QColor color = Qt::white);
    void setMusic(unsigned int number);
    void playMusic();
    void stopMusic(bool resetLength = true, bool resetPlayingEdit = true);
    int setTimePlayedText(int value);
    void splitSongs();
    void putSongsTogether();
    void setMusicNamesToListWidget();
    void setMusicNameToEdit();
    QString getCurrentSongName();
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
private slots:
    void onListWidgetClicked(const QModelIndex &index);
    void onListWidgetActionClicked([[maybe_unused]] bool triggered);
    void onOpenPlaylistButtonClicked();
    void onRemovePlaylistsButtonClicked();
    void onSavePlaylistButtonClicked();
    void onReturnAtEditPressed();
    void onOpenFileButtonClicked();
    void onClosePlaylistButtonClicked();
    void onPlayPauseButtonClicked();
    void onStopButtonClicked();
    void onPreviousButtonClicked();
    void onNextButtonClicked();
    void onRepeatCheckBoxClicked(bool checked);
    void onMusicTimeout();
    void onSliderTimeout();
    void onSliderReleased();
    void onStatusTimeout();
};
#endif // MAINWINDOW_HPP
