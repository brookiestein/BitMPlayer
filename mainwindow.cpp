#include "mainwindow.hpp"
#include "./ui_mainwindow.h"

#include <QAction>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QList>
#include <QStandardPaths>
#include <QStringListModel>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_firstTime(true)
    , m_musicCount(0)
{
    ui->setupUi(this);
    ui->playingEdit->setToolTip(tr("If you wrote the file path yourself, press enter afterwards."));
    ui->playedTimeSlider->setToolTip(tr("Seek music to a certain position."));
    ui->repeatCheckBox->setToolTip(tr("Current music does not repeat."));

    m_playlist = new Playlist(this);

    QList<QAction *> actions;
    actions.append(new QAction(tr("Play this song"), this));
    actions.append(new QAction(tr("Remove from playlist"), this));

    for (auto *action : actions) {
        connect(action, &QAction::triggered, this, &MainWindow::onListWidgetActionClicked);
    }

    ui->listWidget->setContextMenuPolicy(Qt::ActionsContextMenu);
    ui->listWidget->addActions(actions);
    ui->listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->listWidget->setSelectionBehavior(QAbstractItemView::SelectItems);
    ui->listWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

    connect(ui->listWidget, &QAbstractItemView::doubleClicked, this, &MainWindow::onListWidgetClicked);
    connect(ui->playingEdit, &QLineEdit::returnPressed, this, &MainWindow::onReturnAtEditPressed);
    connect(ui->openFilesButton, &QPushButton::clicked, this, &MainWindow::onOpenFileButtonClicked);
    connect(ui->closePlaylistButton, &QPushButton::clicked, this, &MainWindow::onClosePlaylistButtonClicked);
    connect(ui->openPlayListButton, &QPushButton::clicked, this, &MainWindow::onOpenPlaylistButtonClicked);
    connect(ui->removePlayListsButton, &QPushButton::clicked, this, &MainWindow::onRemovePlaylistsButtonClicked);
    connect(ui->savePlayListButton, &QPushButton::clicked, this, &MainWindow::onSavePlaylistButtonClicked);
    connect(ui->playPauseButton, &QPushButton::clicked, this, &MainWindow::onPlayPauseButtonClicked);
    connect(ui->stopButton, &QPushButton::clicked, this, &MainWindow::onStopButtonClicked);
    connect(ui->previousButton, &QPushButton::clicked, this, &MainWindow::onPreviousButtonClicked);
    connect(ui->nextButton, &QPushButton::clicked, this, &MainWindow::onNextButtonClicked);
    connect(ui->repeatCheckBox, &QCheckBox::clicked, this, &MainWindow::onRepeatCheckBoxClicked);
    connect(ui->playedTimeSlider, &QSlider::sliderReleased, this, &MainWindow::onSliderReleased);

    m_musicTimer.setInterval(1); /* Update every millisecond. This helps to get a smooth music update */
    m_sliderTimer.setInterval(1'000); /* Update slider every second */
    m_statusTimer.setInterval(5'000); /* Show status message for 5 seconds */
    connect(&m_musicTimer, &QTimer::timeout, this, &MainWindow::onMusicTimeout);
    connect(&m_sliderTimer, &QTimer::timeout, this, &MainWindow::onSliderTimeout);
    connect(&m_statusTimer, &QTimer::timeout, this, &MainWindow::onStatusTimeout);

    SetTraceLogLevel(LOG_ERROR);
    InitAudioDevice();
}

MainWindow::~MainWindow()
{
    CloseAudioDevice();
    delete ui;
}

void MainWindow::onListWidgetClicked(const QModelIndex &index)
{
    stopMusic();
    m_musicCount = index.row();
    setMusic(m_musicCount);
    setMusicNameToEdit();
    playMusic();
}

void MainWindow::onListWidgetActionClicked(bool triggered)
{
    auto index = ui->listWidget->currentIndex();
    if (index.row() < 0) {
        return;
    }

    auto *action = qobject_cast<QAction *>(sender());
    if (action->text() == tr("Play this song")) {
        stopMusic();
        m_musicCount = index.row();
        setMusic(m_musicCount);
        setMusicNameToEdit();
        playMusic();
        return;
    }

    /* Working with remove from playlist action */
    auto filename = index.data().toString();
    auto path = m_splittedSongs[filename];
    m_filenames.removeOne(QString("%1%2%3").arg(path, QDir::separator(), filename));
    m_splittedSongs.erase(filename);
    ui->listWidget->model()->removeRow(index.row());

    /* Because m_musicPlaying holds the full file path of the currently playing music
     * and the list view just shows the filename.
     */
    auto idx = m_musicPlaying.lastIndexOf('/');
    auto musicPlayingFilename = m_musicPlaying.mid(idx + 1, m_musicPlaying.size());

    if (musicPlayingFilename == filename) {
        stopMusic();
        if (ui->playingEdit->text() == filename) {
            ui->playingEdit->setText("");
        }

        if (m_musicCount > index.row()) {
            m_musicCount = index.row();
        }

        setMusic(m_musicCount);
        ui->listWidget->setCurrentRow(m_musicCount);
        /* TODO: Check if setMusic() shall ui->playingEdit->setText() */
        idx = m_musicPlaying.lastIndexOf('/');
        musicPlayingFilename = m_musicPlaying.mid(idx + 1, m_musicPlaying.size());
        ui->playingEdit->setText(musicPlayingFilename);
    }
}

void MainWindow::onOpenPlaylistButtonClicked()
{
    m_splittedSongs = m_playlist->openPlayList();
    if (m_splittedSongs.size() == 0) {
        return;
    }

    m_musicCount = 0;
    putSongsTogether();
    setMusicNamesToListWidget();
    setMusicNameToEdit();
    setMusic(m_musicCount);
}

void MainWindow::onRemovePlaylistsButtonClicked()
{
    int removed = m_playlist->removePlaylists();
    if (removed == 0) {
        return;
    }

    auto message = removed == 1 ? tr("%1 playlist removed.") : tr("%1 playlists removed.");
    message = message.arg(QString::number(removed));
    setStatusText(message);
}

void MainWindow::onSavePlaylistButtonClicked()
{
    if (m_filenames.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("No song has been loaded."));
        return;
    }

    auto playlistName = QInputDialog::getText(this, tr("Playlist Name"), tr("Enter playlist name:"));
    if (playlistName.isEmpty()) {
        setStatusText(tr("Playlist not saved."), Qt::red);
        return;
    }

    m_playlist->savePlayList(playlistName, m_splittedSongs);
    setStatusText(tr("Playlist saved!"), Qt::green);
}

void MainWindow::resetControllers(bool resetLength, bool resetPlayingEdit)
{
    ui->playPauseButton->setText(tr("Play"));
    ui->playedTimeSlider->setSingleStep(0);
    ui->playedTimeSlider->setValue(0);
    ui->timePlayedLabel->setText("0s");

    if (resetLength)
        ui->lengthLabel->setText("0m");

    if (resetPlayingEdit)
        ui->playingEdit->setText("");
}

void MainWindow::setStatusText(QString text, QColor color)
{
    ui->statusLabel->setText(text);
    auto palette = ui->statusLabel->palette();
    palette.setColor(ui->statusLabel->foregroundRole(), color);
    ui->statusLabel->setPalette(palette);
    m_statusTimer.start();
}

void MainWindow::setMusic(unsigned int number)
{
    if (number >= m_filenames.count()) {
        return;
    }

    m_musicPlaying = m_filenames[number];
    m_music = LoadMusicStream(m_musicPlaying.toStdString().c_str());
    m_music.looping = false;
    m_streamLength = GetMusicTimeLength(m_music);
    auto length = static_cast<int>(m_streamLength);
    int maximum {length};
    QString text {};

    if (length >= 60) {
        auto minutes = length / 60;
        length %= 60;
        if (minutes >= 60) {
            auto hours = minutes / 60;
            minutes %= 60;
            length %= 60;
            text = tr("%1h, %2m, %3s")
                       .arg(QString::number(hours), QString::number(minutes), QString::number(length));
        } else {
            text = tr("%1m, %2s").arg(QString::number(minutes), QString::number(length));
        }
    } else {
        text = tr("%1s").arg(QString::number(length));
    }

    ui->lengthLabel->setText(text);
    ui->playedTimeSlider->setMaximum(maximum);
    ui->playedTimeSlider->setValue(0);
}

void MainWindow::playMusic()
{
    PlayMusicStream(m_music);
    m_musicTimer.start();
    m_sliderTimer.start();
    ui->playPauseButton->setText(tr("Pause"));
    m_firstTime = false;
}

void MainWindow::stopMusic(bool resetLength, bool resetPlayingEdit)
{
    m_musicTimer.stop();
    m_sliderTimer.stop();

    /* ResumeMusicStream for StopMusicStream to work properly.
     * If user pauses the song and afterward stops it, GetMusicTimePlayed
     * will return the last time played rather than 0.0f which is the
     * correct value taking in account that what we actually want is stop the song.
     * In other words, reset it.
     */
    if (not IsMusicStreamPlaying(m_music))
        ResumeMusicStream(m_music);

    StopMusicStream(m_music);
    resetControllers(resetLength, resetPlayingEdit);
    m_firstTime = true;
}

int MainWindow::setTimePlayedText(int value)
{
    QString text {};

    if (value >= 60) {
        auto minutes = value / 60;
        value %= 60;
        if (minutes >= 60) {
            auto hours = minutes / 60;
            minutes %= 60;
            value %= 60;
            text = tr("%1h, %2m, %3s")
                       .arg(QString::number(hours), QString::number(minutes), QString::number(value));
        } else {
            text = tr("%1m, %2s").arg(QString::number(minutes), QString::number(value));
        }
    } else {
        text = tr("%1s").arg(QString::number(value));
    }

    ui->timePlayedLabel->setText(text);

    return value;
}

void MainWindow::splitSongs()
{
    for (auto file : m_filenames) {
        auto index = file.lastIndexOf('/');
        auto songName = file.mid(index + 1, file.size());
        auto filePath = file.mid(0, index);
        m_splittedSongs[songName] = filePath;
    }
}

void MainWindow::putSongsTogether()
{
    for (const auto [songName, filePath] : m_splittedSongs) {
        m_filenames << QString("%1%2%3").arg(filePath, QDir::separator(), songName);
    }
}

void MainWindow::setMusicNamesToListWidget()
{
    QStringList names;

    for (const auto [songName, filePath] : m_splittedSongs) {
        names << songName;
    }

    if (ui->listWidget->count() > 0) {
        ui->listWidget->clear();
    }

    ui->listWidget->addItems(names);
}

void MainWindow::setMusicNameToEdit()
{
    ui->playingEdit->setText(getCurrentSongName());
}

QString MainWindow::getCurrentSongName()
{
    QString songName {};
    int i {};
    for (auto it = m_splittedSongs.begin(); it != m_splittedSongs.end(); ++it, ++i) {
        if (i == m_musicCount) {
            songName = it->first;
            break;
        }
    }

    return songName;
}

void MainWindow::onReturnAtEditPressed()
{
    auto filename = ui->playingEdit->text();
    auto index = filename.lastIndexOf('/');
    auto songName = filename.mid(index + 1, filename.size());
    auto filePath = filename.mid(0, index + 1);

    m_filenames.append(filename);
    m_splittedSongs[songName] = filePath;
    setMusic(m_filenames.indexOf(m_filenames.last()));
    setMusicNameToEdit();
    setMusicNamesToListWidget();
    ui->listWidget->setCurrentRow(m_filenames.indexOf(m_filenames.last()));
}

void MainWindow::onOpenFileButtonClicked()
{
    auto files = QFileDialog::getOpenFileNames(this,
                                                 tr("Open audio files"),
                                                 QStandardPaths::writableLocation(QStandardPaths::MusicLocation),
                                                 tr("Audio files (*.mp3 *.ogg *.wav *.qoa *.flac *.xm *.mod)"));

    if (files.isEmpty()) {
        return;
    }

    for (auto file : files) {
        m_filenames.append(file);
    }

    m_filenames.sort();
    splitSongs();

    bool restoreTimePlayed {};
    if (not m_musicPlaying.isEmpty()) {
        m_lastMusicPlaying = m_musicPlaying;
        m_lastTimePlayed = GetMusicTimePlayed(m_music);
        m_musicCount = m_filenames.indexOf(m_lastMusicPlaying);
        stopMusic(false, false);
        restoreTimePlayed = true;
    }

    setMusicNamesToListWidget();
    setMusic(m_musicCount);
    setMusicNameToEdit();
    ui->listWidget->setCurrentRow(m_filenames.indexOf(m_musicPlaying));

    if (restoreTimePlayed) {
        SeekMusicStream(m_music, m_lastTimePlayed);
        int timePlayed = static_cast<int>(m_lastTimePlayed);
        ui->playedTimeSlider->setValue(timePlayed);
        setTimePlayedText(timePlayed);
        playMusic();
    }
}

void MainWindow::onClosePlaylistButtonClicked()
{
    if (m_filenames.isEmpty()) {
        return;
    }

    stopMusic();

    m_filenames.clear();
    m_splittedSongs.clear();
    m_musicCount = 0;
    m_musicPlaying.clear();
    ui->playingEdit->setText("");
    resetControllers();

    int rowCount = ui->listWidget->model()->rowCount();
    for (int i {}; i < rowCount; ++i) {
        ui->listWidget->model()->removeRow(0);
    }
}

void MainWindow::onPlayPauseButtonClicked()
{
    static bool isTheFirstTime {true};
    if (m_filenames.isEmpty() or not IsMusicReady(m_music)) {
        QMessageBox::warning(this, tr("Warning"), tr("No song has been loaded."));
        return;
    }

    if (isTheFirstTime or m_firstTime) {
        playMusic();
        isTheFirstTime = false;
        m_firstTime = false;
        return;
    }

    auto *button = qobject_cast<QPushButton *>(sender());
    if (button->text() == tr("Play")) {
        ResumeMusicStream(m_music);
        button->setText(tr("Pause"));
        m_musicTimer.start();
        m_sliderTimer.start();
    } else {
        PauseMusicStream(m_music);
        button->setText(tr("Play"));
        m_musicTimer.stop();
        m_sliderTimer.stop();
    }
}

void MainWindow::onStopButtonClicked()
{
    if (GetMusicTimePlayed(m_music) > 0.0f)
        stopMusic(false, false);
}

void MainWindow::onPreviousButtonClicked()
{
    if (m_filenames.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("No song has been loaded."));
        return;
    }

    stopMusic(false, false);

    if (m_musicCount <= 0) {
        QMessageBox::warning(this, tr("No Music to Play"), tr("There's no previous music to play."));
        return;
    }

    setMusic(--m_musicCount);
    playMusic();
    setMusicNameToEdit();
    ui->listWidget->setCurrentRow(m_filenames.indexOf(m_musicPlaying));
}

void MainWindow::onNextButtonClicked()
{
    if (m_filenames.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("No file has been loaded."));
        return;
    }

    stopMusic(false, false);

    if ((m_musicCount + 1) >= m_filenames.count()) {
        QMessageBox::warning(this, tr("No Music to Play"), tr("There's no next music to play."));
        return;
    }

    setMusic(++m_musicCount);
    playMusic();
    setMusicNameToEdit();
    ui->listWidget->setCurrentRow(m_filenames.indexOf(m_musicPlaying));
}

void MainWindow::onRepeatCheckBoxClicked(bool checked)
{
    if (m_filenames.isEmpty()) {
        if (checked) {
            ui->repeatCheckBox->setChecked(false);
        }
    } else {
        m_music.looping = checked;
        if (checked) {
            ui->repeatCheckBox->setToolTip(tr("Current music repeats."));
        } else {
            ui->repeatCheckBox->setToolTip(tr("Current music does not repeat."));
        }
    }
}

void MainWindow::onMusicTimeout()
{
    UpdateMusicStream(m_music);
    auto timePlayed = static_cast<int>(GetMusicTimePlayed(m_music));
    setTimePlayedText(timePlayed);

    if (not IsMusicStreamPlaying(m_music)) {
        stopMusic(false, false);

        if (m_musicCount + 1 < m_filenames.count()) {
            qDebug() << "Playing next song:" << m_filenames[m_musicCount + 1];
            setMusic(++m_musicCount);
            setMusicNameToEdit();
            playMusic();
        }
    } else if (static_cast<int>(GetMusicTimePlayed(m_music)) == static_cast<int>(m_streamLength)) {
        resetControllers(false, false);
    } else {
        ui->playPauseButton->setText(tr("Pause"));
    }
}

void MainWindow::onSliderTimeout()
{
    ui->playedTimeSlider->setValue(ui->playedTimeSlider->value() + 1);
}

void MainWindow::onSliderReleased()
{
    if (m_filenames.isEmpty()) {
        return;
    }

    int value = ui->playedTimeSlider->value();

    setTimePlayedText(value);
    SeekMusicStream(m_music, static_cast<float>(value));
}

void MainWindow::onStatusTimeout()
{
    ui->statusLabel->setText("");
    auto palette = ui->statusLabel->palette();
    palette.setColor(ui->statusLabel->foregroundRole(), Qt::white);
    ui->statusLabel->setPalette(palette);
    m_statusTimer.stop();
}
