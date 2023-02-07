/*
 *  Fernanda is a plain text editor for drafting long-form fiction. (At least, that's the plan.)
 *  Copyright (C) 2022-2023 @fairybow <https://github.com/fairybow>
 *
 *  <https://github.com/fairybow/fernanda>
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 */

// pane.h, Fernanda

#pragma once

#include "delegate.h"
#include "io.h"
#include "text.h"

#include <QAbstractItemView>
#include <QAction>
#include <QContextMenuEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QInputDialog>
#include <QMenu>
#include <QMouseEvent>
#include <QPoint>
#include <QResizeEvent>
#include <QScrollBar>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTreeView>

class Pane : public QTreeView
{
    Q_OBJECT

public:
    Pane(QWidget* parent = nullptr);

    enum class Go {
        Next,
        Previous
    };

    void navigate(Go direction);

    const QStringList devGetEditedKeys() { return delegate->paintEdited; }

public slots:
    void receiveItems(QVector<QStandardItem*> items);
    void receiveEditsList(QStringList editedFiles);

    void add(Path::Type type) { addTempItem(type, QPoint(-1, -1)); }

protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    QStandardItemModel* itemModel = new QStandardItemModel(this);
    PaneDelegate* delegate = new PaneDelegate(this);

    void expandItems_recursor(QStandardItem* item);
    void addTempItem(Path::Type type, QPoint eventPosition);
    QStandardItem* tempItem(Path::Type type);
    const QString rename();

    void refresh() { dataChanged(QModelIndex(), QModelIndex()); }

private slots:
    void onClick(const QModelIndex& index);

signals:
    void askAddElement(QString newName, Path::Type type, QString parentKey);
    void askCutElement(QString key);
    void askDomMove(QString pivotKey, QString fulcrumKey, Io::Move position);
    bool askHasProject();
    void askTitleCheck();
    void askRenameElement(QString newName, QString key);
    void askSendToEditor(QString key);
    void askSetExpansion(QString key, bool isExpanded);
};

// pane.h, Fernanda
