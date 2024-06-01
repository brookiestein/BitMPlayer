#include <QStringListModel>

#include "playlistselector.hpp"
#include "ui_playlistselector.h"

PlaylistSelector::PlaylistSelector(QStringList playlistNames,
                                   QAbstractItemView::SelectionMode selectionMode,
                                   QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PlaylistSelector)
    , m_playlistNames(playlistNames)
{
    ui->setupUi(this);

    auto *model = new QStringListModel(playlistNames, this);
    ui->listView->setModel(model);
    ui->listView->setEditTriggers(QListView::NoEditTriggers);
    ui->listView->setSelectionMode(selectionMode);
    ui->listView->setSelectionBehavior(QAbstractItemView::SelectItems);

    connect(ui->listView, &QListView::doubleClicked, this, &PlaylistSelector::doubleClicked);
    connect(ui->selectButton, &QPushButton::clicked, this, &PlaylistSelector::onSelectButtonClicked);
}

PlaylistSelector::~PlaylistSelector()
{
    delete ui;
}

QStringList PlaylistSelector::getSelection() const
{
    return m_selection;
}

void PlaylistSelector::closeEvent(QCloseEvent *event)
{
    emit closed();
    QWidget::closeEvent(event);
}

void PlaylistSelector::doubleClicked(const QModelIndex &index)
{
    m_selection.append(index.data().toString());
    close();
}

void PlaylistSelector::onSelectButtonClicked()
{
    auto selectedIndexes = ui->listView->selectionModel()->selectedIndexes();
    for (auto selectedIndex : selectedIndexes) {
        m_selection << selectedIndex.data().toString();
    }

    close();
}
