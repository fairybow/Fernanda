/*
*   Fernanda is a plain text editor for drafting long-form fiction. (At least, that's the plan.)
*   Copyright(C) 2022 - 2023  @fairybow (https://github.com/fairybow)
*
*   https://github.com/fairybow/fernanda
*
*   You should have received a copy of the GNU General Public License
*   along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

// pane.cpp, Fernanda

#include "pane.h"

Pane::Pane(QWidget* parent)
    : QTreeView(parent)
{
    setObjectName("pane");
    horizontalScrollBar()->setObjectName("hScrollBar");
    verticalScrollBar()->setObjectName("pvScrollBar");
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setItemDelegate(delegate);
    setModel(itemModel);
    setHeaderHidden(true);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setExpandsOnDoubleClick(true);
    setDragEnabled(true);
    viewport()->setAcceptDrops(false);
    setDropIndicatorShown(true);
    setDragDropMode(QAbstractItemView::InternalMove);
    setContextMenuPolicy(Qt::DefaultContextMenu);
    connect(this, &Pane::clicked, this, &Pane::onClick);
    connect(this, &Pane::expanded, this, [&](const QModelIndex& index)
        {
            askSetExpansion(Index::key(index), true);
            askTitleCheck();
        });
    connect(this, &Pane::collapsed, this, [&](const QModelIndex& index)
        {
            askSetExpansion(Index::key(index), false);
            askTitleCheck();
        });
}

void Pane::navigate(Go direction)
{
    auto current_index = currentIndex();
    if (!isExpanded(current_index))
        expand(current_index);
    QModelIndex next;
    (direction == Go::Previous)
        ? next = indexAbove(current_index)
        : next = indexBelow(current_index);
    if (next.isValid())
        setCurrentIndex(next);
    else
    {
        setCurrentIndex(model()->index(0, 0));
        auto valid = true;
        while (valid)
        {
            auto last_valid_index = currentIndex();
            QModelIndex wrap_around;
            (direction == Go::Previous)
                ? wrap_around = indexBelow(last_valid_index)
                : wrap_around = indexAbove(last_valid_index);
            if (wrap_around.isValid())
                setCurrentIndex(wrap_around);
            else
                valid = false;
        }
    }
    auto destination_index = currentIndex();
    auto destination_index_child_rows = itemModel->itemFromIndex(destination_index)->rowCount();
    if (destination_index_child_rows && !isExpanded(destination_index))
    {
        expand(destination_index);
        if (direction == Go::Previous)
            for (auto i = 0; i < destination_index_child_rows; ++i)
            {
                destination_index = indexBelow(destination_index);
                setCurrentIndex(destination_index);
            }
    }
    if (Index::isDir(destination_index)) return;
    clicked(destination_index);
}

void Pane::receiveItems(QVector<QStandardItem*> items)
{
    itemModel->clear();
    for (auto& item : items)
    {
        itemModel->appendRow(item);
        expandItems_recursor(item);
    }
}

void Pane::receiveEditsList(QStringList editedFiles)
{
    if (editedFiles == delegate->paintEdited) return;
    delegate->paintEdited = editedFiles;
    refresh();
}

void Pane::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (currentIndex().isValid() && currentIndex().model()->hasChildren())
        QTreeView::mouseDoubleClickEvent(event);
}

void Pane::dropEvent(QDropEvent* event)
{
    auto drop = dropIndicatorPosition();
    auto pivot = currentIndex();
    auto fulcrum = indexAt(event->pos());
    Io::Move position{};
    switch (drop) {
    case QAbstractItemView::AboveItem:
        position = Io::Move::Above;
        break;
    case QAbstractItemView::BelowItem:
        position = Io::Move::Below;
        break;
    case QAbstractItemView::OnItem:
        if (Index::isDir(pivot) && Index::isFile(fulcrum)) return;
        position = Io::Move::On;
        expand(fulcrum);
        break;
    case QAbstractItemView::OnViewport:
        position = Io::Move::Viewport;
        break;
    }
    askDomMove(Index::key(pivot), Index::key(fulcrum), position);
    event->ignore();
}

void Pane::contextMenuEvent(QContextMenuEvent* event)
{
    auto project = askHasProject();
    if (!project) return;
    auto& position = event->pos();
    auto index = indexAt(position);
    auto menu = new QMenu(this);
    auto rename_item = new QAction(tr("&Rename"), this);
    auto cut_item = new QAction(tr("&Cut"), this);
    auto new_folder = new QAction(tr("&New folder"), this);
    auto new_file = new QAction(tr("&New file"), this);
    connect(rename_item, &QAction::triggered, this, [&]()
        {
            askRenameElement(rename(), Index::key(currentIndex()));
        });
    connect(cut_item, &QAction::triggered, this, [&]()
        {
            askCutElement(Index::key(currentIndex()));
        });
    connect(new_folder, &QAction::triggered, this, [&]()
        {
            addTempItem(Path::Type::Dir, position);
        });
    connect(new_file, &QAction::triggered, this, [&]()
        {
            addTempItem(Path::Type::File, position);
        });
    if (Index::isFile(index))
    {
        menu->addAction(rename_item);
        menu->addSeparator();
        menu->addAction(new_file);
        menu->addSeparator();
        menu->addAction(cut_item);
    }
    else if (Index::isDir(index))
    {
        menu->addAction(rename_item);
        menu->addSeparator();
        menu->addAction(new_folder);
        menu->addAction(new_file);
        menu->addSeparator();
        menu->addAction(cut_item);
    }
    else
    {
        menu->addAction(new_folder);
        menu->addAction(new_file);
    }
    menu->exec(event->globalPos());
}

void Pane::resizeEvent(QResizeEvent* event)
{
    delegate->paneSize = event->size();
    QTreeView::resizeEvent(event);
    refresh();
}

void Pane::expandItems_recursor(QStandardItem* item)
{
    auto index = item->index();
    if (Index::isExpanded(index))
        setExpanded(index, true);
    if (item->hasChildren())
        for (auto i = 0; i < item->rowCount(); ++i)
            expandItems_recursor(item->child(i));
}

void Pane::addTempItem(Path::Type type, QPoint eventPosition)
{
    auto temp_item = tempItem(type);
    auto parent_index = indexAt(eventPosition);
    switch (type) {
    case Path::Type::Dir:
        if (parent_index.isValid() && Index::isDir(parent_index))
            itemModel->itemFromIndex(parent_index)->appendRow(temp_item);
        else
            itemModel->appendRow(temp_item);
        break;
    case Path::Type::File:
        (parent_index.isValid())
            ? itemModel->itemFromIndex(parent_index)->appendRow(temp_item)
            : itemModel->appendRow(temp_item);
        break;
    }
    if (parent_index.isValid())
        expand(parent_index);
    temp_item->setEnabled(false);
    QString parent_key;
    if (parent_index.isValid())
        parent_key = Index::key(parent_index);
    askAddElement(rename(), type, parent_key);
}

QStandardItem* Pane::tempItem(Path::Type type)
{
    QStandardItem* result = new QStandardItem();
    switch (type) {
    case Path::Type::Dir:
        result->setData("directory", Qt::UserRole);
        break;
    case Path::Type::File:
        result->setData("file", Qt::UserRole);
        break;
    }
    result->setData("Untitled", Qt::UserRole + 1);
    return result;
}

const QString Pane::rename()
{
    bool has_input = false;
    QString text = QInputDialog::getText(this, tr(nullptr), tr(nullptr), QLineEdit::Normal, nullptr, &has_input);
    if (has_input && !text.isEmpty())
        return text.replace(Text::regex(Text::Regex::Forbidden), "_");
    return nullptr;
}

void Pane::onClick(const QModelIndex& index)
{
    if (Index::isFile(index))
    {
        auto key = Index::key(index);
        delegate->paintActive = key;
        askSendToEditor(key);
        refresh();
    }
}

// pane.cpp, Fernanda
