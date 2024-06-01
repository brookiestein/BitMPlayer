#ifndef PLAYLISTSELECTOR_HPP
#define PLAYLISTSELECTOR_HPP

#include <QAbstractItemView>
#include <QCloseEvent>
#include <QModelIndex>
#include <QWidget>

namespace Ui {
class PlaylistSelector;
}

class PlaylistSelector : public QWidget
{
    Q_OBJECT
    Ui::PlaylistSelector *ui;
    QStringList m_selection;
    QStringList m_playlistNames;
public:
    explicit PlaylistSelector(QStringList playlistNames,
                              QAbstractItemView::SelectionMode selectionMode = QAbstractItemView::SingleSelection,
                              QWidget *parent = nullptr);
    ~PlaylistSelector();
    QStringList getSelection() const;
protected:
    void closeEvent(QCloseEvent *event) override;
signals:
    void closed();
private slots:
    void doubleClicked(const QModelIndex &index);
    void onSelectButtonClicked();
};

#endif // PLAYLISTSELECTOR_HPP
