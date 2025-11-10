/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QAbstractItemModel>
#include <QDomDocument>
#include <QDomElement>
#include <QHash>
#include <QMimeData>
#include <QModelIndex>
#include <QModelIndexList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <Qt>
#include <QtTypes>

#include "Debug.h"
#include "Fnx.h"

#include "Coco/Path.h"

namespace Fernanda {

// See:
// https://doc.qt.io/qt-6/model-view-programming.html#model-subclassing-reference
//
// Read-only Qt Model/View adapter for .fnx virtual file structure. Presents a
// QDomDocument as a tree structure for QTreeView. Translates DOM hierarchy into
// Qt indices and provides display data. Does NOT modify the DOM or perform I/O
// - those are Notebook and Fnx responsibilities. (TODO: Is this reasonable?)
//
// Workflow: Notebook loads DOM via Fnx -> gives to FnxModel -> FnxModel
// presents to UI -> Notebook modifies DOM via Fnx -> refreshes FnxModel. (TODO:
// Check this later)
//
// TODO: Trash (should be immutable and separate from active)
// TODO: Double clicking on files should maybe not expand (if they have
// children), since they also open with double clicks?
class FnxModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    FnxModel(QObject* parent)
        : QAbstractItemModel(parent)
    {
    }

    virtual ~FnxModel() override { TRACER; }

    void initialize(const QDomDocument& dom)
    {
        if (initialized_) return;
        // beginResetModel();
        dom_ = dom;
        // elementToId_.clear();
        // idToElement_.clear();
        // nextId_ = 1; // Reserve 0 for root
        // endResetModel();
        initialized_ = true;
    }

    QDomDocument domDocument() const { return dom_; }

    // QDomNode/QDomElement function like views onto the same underlying DOM
    // document, so copies are relatively cheap (because it's like passing a
    // handle) and modifying a copy modifies the underlying shared DOM data.
    // This is why parentElement is passed by value but still modifies the
    // actual document when appendChild is called.
    // TODO: Expand parent if applicable after appending
    void insertElement(const QDomElement& element, QDomElement parentElement)
    {
        if (element.isNull() || parentElement.isNull()) return;

        // Get parent's index (invalid index if parent is root)
        auto parent_index = indexFromElement_(parentElement);

        // Count existing children to determine insertion row
        auto row = 0;
        auto child = parentElement.firstChildElement();
        while (!child.isNull()) {
            ++row;
            child = child.nextSiblingElement();
        }

        beginInsertRows(parent_index, row, row);
        parentElement.appendChild(element);
        endInsertRows();

        emit domChanged();
    }

    // TODO: Expand parent if applicable after appending
    // TODO: Use commented-out version below
    void insertElements(
        const QList<QDomElement>& elements,
        QDomElement parentElement)
    {
        if (elements.isEmpty() || parentElement.isNull()) return;

        for (const auto& element : elements) {
            if (element.isNull()) continue;

            auto parent_index = indexFromElement_(parentElement);
            auto row = 0;
            auto child = parentElement.firstChildElement();
            while (!child.isNull()) {
                ++row;
                child = child.nextSiblingElement();
            }

            beginInsertRows(parent_index, row, row);
            parentElement.appendChild(element);
            endInsertRows();
        }

        emit domChanged();
    }

    /*void insertElements(
    const QList<QDomElement>& elements,
    QDomElement parentElement)
    {
        if (elements.isEmpty() || parentElement.isNull()) return;

        auto parent_index = indexFromElement_(parentElement);
        auto row = childElementCount_(parentElement);  // Count once

        for (const auto& element : elements) {
            if (element.isNull()) continue;

            beginInsertRows(parent_index, row, row);
            parentElement.appendChild(element);
            endInsertRows();
            ++row;  // Increment for next insertion
        }

        emit domChanged();
    }*/

    // void updateElement(const QDomElement& element)
    // {
    //     if (element.isNull()) return;
    //     auto index = indexFromElement_(element);
    //     if (!index.isValid()) return;
    //     // QAbstractItemModel::dataChanged in automatically connected when
    //     you
    //     // set a view's model
    //     emit dataChanged(index, index);
    //     emit domChanged();
    // }

    // TODO: Debug version, to catch the QPersistentModelIndex crash
    // TODO: Check logic here vs below, side by side
    bool
    moveElement(const QDomElement& element, QDomElement newParent, int newRow)
    {
        DEBUG("=== MOVE ELEMENT START ===");
        DEBUG("  element: {}", element);
        DEBUG("  newParent: {}", newParent);
        DEBUG("  newRow: {}", newRow);

        if (element.isNull() || newParent.isNull()) {
            WARN(
                "Early return: element.isNull()={}, newParent.isNull()={}",
                element.isNull(),
                newParent.isNull());
            return false;
        }

        auto current_parent = element.parentNode().toElement();
        DEBUG("  current_parent: {}", current_parent);

        if (current_parent.isNull()) {
            WARN("Early return: current_parent.isNull()=true");
            return false;
        }

        if (isDescendantOf_(element, newParent)) {
            WARN("Early return: newParent is descendant of element");
            return false;
        }

        auto source_parent_index = indexFromElement_(current_parent);
        auto source_row = rowOfElement_(element);
        auto dest_parent_index = indexFromElement_(newParent);

        DEBUG(
            "  source_parent_index: {}, valid={}",
            source_parent_index,
            source_parent_index.isValid());
        DEBUG("  source_row: {}", source_row);
        DEBUG(
            "  dest_parent_index: {}, valid={}",
            dest_parent_index,
            dest_parent_index.isValid());

        auto dest_row = newRow;
        if (dest_row < 0) {
            dest_row = childElementCount_(newParent);
            DEBUG("  dest_row adjusted from {} to {}", newRow, dest_row);
        } else {
            DEBUG("  dest_row: {}", dest_row);
        }

        if (current_parent == newParent && source_row == dest_row) {
            DEBUG("No-op move: same parent and same row");
            return true;
        }

        DEBUG(
            "Calling beginMoveRows(srcParent={}, srcRow={}, srcRow={}, "
            "destParent={}, destRow={})",
            source_parent_index,
            source_row,
            source_row,
            dest_parent_index,
            dest_row);

        if (!beginMoveRows(
                source_parent_index,
                source_row,
                source_row,
                dest_parent_index,
                dest_row)) {
            WARN("beginMoveRows FAILED - returning false");
            return false;
        }

        DEBUG("beginMoveRows succeeded");

        // Perform DOM manipulation
        DEBUG("Before removeChild - element: {}", element);
        current_parent.removeChild(element);
        DEBUG("After removeChild - element: {}", element);

        auto child_count = childElementCount_(newParent);
        DEBUG("newParent child count before insertion: {}", child_count);

        if (dest_row >= child_count) {
            DEBUG(
                "Appending element (dest_row {} >= child_count {})",
                dest_row,
                child_count);
            newParent.appendChild(element);
        } else {
            auto sibling = nthChildElement_(newParent, dest_row);
            DEBUG(
                "Inserting before sibling at position {}: {}",
                dest_row,
                sibling);
            if (!sibling.isNull()) {
                newParent.insertBefore(element, sibling);
            } else {
                WARN("Sibling was null, appending instead");
                newParent.appendChild(element);
            }
        }

        DEBUG("After insertion - element: {}", element);

        endMoveRows();
        DEBUG("endMoveRows called");

        emit domChanged();
        DEBUG("=== MOVE ELEMENT END (success) ===");

        return true;
    }

    /*bool
    moveElement(const QDomElement& element, QDomElement newParent, int newRow)
    {
        if (element.isNull() || newParent.isNull()) return false;

        auto current_parent = element.parentNode().toElement();
        if (current_parent.isNull()) return false;

        if (isDescendantOf_(element, newParent)) return false;

        auto source_parent_index = indexFromElement_(current_parent);
        auto source_row = rowOfElement_(element);
        auto dest_parent_index = indexFromElement_(newParent);

        auto dest_row = newRow;
        if (dest_row < 0) {
            dest_row = childElementCount_(newParent);
        }

        // No adjustment - Qt handles this correctly

        // After calculating positions, before beginMoveRows:
        if (current_parent == newParent && source_row == dest_row) {
            return true; // No move needed
        }

        if (!beginMoveRows(
                source_parent_index,
                source_row,
                source_row,
                dest_parent_index,
                dest_row)) {
            return false;
        }

        // Perform DOM manipulation
        current_parent.removeChild(element);

        // After removal, dest_row might be out of bounds, which is fine
        if (dest_row >= childElementCount_(newParent)) {
            newParent.appendChild(element);
        } else {
            auto sibling = nthChildElement_(newParent, dest_row);
            if (!sibling.isNull()) {
                newParent.insertBefore(element, sibling);
            } else {
                newParent.appendChild(element);
            }
        }

        endMoveRows();
        emit domChanged();

        return true;
    }*/

    // TODO: moveElements (maybe)

    QDomElement elementAt(const QModelIndex& index) const
    {
        if (!index.isValid()) return dom_.documentElement();

        auto id = reinterpret_cast<quintptr>(index.internalPointer());
        return idToElement_.value(id);
    }

    virtual Qt::ItemFlags flags(const QModelIndex& index) const override
    {
        auto default_flags = QAbstractItemModel::flags(index);

        if (!index.isValid()) {
            // Root can accept drops but isn't draggable/editable
            return default_flags | Qt::ItemIsDropEnabled;
        }

        // All items are draggable, droppable, and editable
        return default_flags | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled
               | Qt::ItemIsEditable;
    }

    virtual bool setData(
        const QModelIndex& index,
        const QVariant& value,
        int role = Qt::EditRole) override
    {
        if (!index.isValid()) return false;

        auto element = elementAt(index);
        if (element.isNull()) return false;

        switch (role) {
        case Qt::EditRole:
        case Qt::DisplayRole: {
            // Handle rename
            auto new_name = value.toString();
            if (new_name.isEmpty()) return false; // Reject empty names

            Fnx::rename(element, new_name);
            emit dataChanged(index, index, { Qt::DisplayRole, Qt::EditRole });
            emit elementRenamed(element);
            emit domChanged();

            return true;
        }

        // Future: Handle other roles
        case Qt::DecorationRole:
            // Could handle custom icons per element
            return false;

        case Qt::ToolTipRole:
            // Could store custom tooltips in XML attributes
            return false;

        case Qt::CheckStateRole:
            // Could add checked/unchecked state to elements
            return false;

        default:
            return false;
        }
    }

    virtual QModelIndex
    index(int row, int column, const QModelIndex& parent = {}) const override
    {
        if (!hasIndex(row, column, parent)) return {};

        auto parent_element = elementAt(parent);
        auto child_element = nthChildElement_(parent_element, row);
        if (child_element.isNull()) return {};

        return createIndex(row, column, idFromElement_(child_element));
    }

    virtual QModelIndex parent(const QModelIndex& child) const override
    {
        if (!child.isValid()) return {};

        auto child_element = elementAt(child);
        auto parent_element = child_element.parentNode().toElement();

        // Root or invalid
        if (parent_element.isNull() || parent_element == dom_.documentElement())
            return {};

        auto row = rowOfElement_(parent_element);
        return createIndex(row, 0, idFromElement_(parent_element));
    }

    virtual int rowCount(const QModelIndex& parent = {}) const override
    {
        if (parent.column() > 0) return 0;

        auto element = elementAt(parent);
        return childElementCount_(element);
    }

    virtual int columnCount(const QModelIndex& parent = {}) const override
    {
        return 1; // TODO: Just "Name" column for now
    }

    virtual QVariant
    data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    // TODO: Not used yet
    virtual Qt::DropActions supportedDragActions() const override
    {
        return Qt::MoveAction;
    }

    virtual Qt::DropActions supportedDropActions() const override
    {
        return Qt::MoveAction;
    }

    virtual QStringList mimeTypes() const override { return { MIME_TYPE_ }; }

    virtual QMimeData* mimeData(const QModelIndexList& indexes) const override
    {
        if (indexes.isEmpty()) return nullptr;

        // TODO: Only support dragging single items for now
        QModelIndex index = indexes.first();
        if (!index.isValid()) return nullptr;

        auto element = elementAt(index);
        if (element.isNull()) return nullptr;

        // Store the element's unique key so we can find it on drop
        auto key = elementKey_(element);
        auto mime_data = new QMimeData;
        mime_data->setData(MIME_TYPE_, key.toUtf8());

        return mime_data;
    }

    virtual bool dropMimeData(
        const QMimeData* data,
        Qt::DropAction action,
        int row,
        int column,
        const QModelIndex& parent) override
    {
        if (!data || action != Qt::MoveAction) return false;
        if (!data->hasFormat(MIME_TYPE_)) return false;

        // Decode the dragged element
        auto key = QString::fromUtf8(data->data(MIME_TYPE_));

        if (!elementToId_.contains(key)) return false;

        auto element_id = elementToId_[key];
        QDomElement element = idToElement_[element_id];

        if (element.isNull()) return false;

        // Determine drop target
        auto drop_parent = elementAt(parent);
        if (drop_parent.isNull()) {
            drop_parent = dom_.documentElement();
        }

        return moveElement(element, drop_parent, row);
    }

signals:
    void domChanged();
    void elementRenamed(const QDomElement& element);

private:
    bool initialized_ = false;
    QDomDocument dom_{};
    static constexpr auto MIME_TYPE_ = "application/x-fernanda-fnx-element";
    mutable QHash<QString, quintptr> elementToId_{}; // Element's UUID -> ID
    mutable QHash<quintptr, QDomElement> idToElement_{}; // ID -> Element
    mutable quintptr nextId_ = 1;

    QString elementKey_(const QDomElement& element) const
    {
        if (element.isNull()) return {};

        auto uuid = Fnx::uuid(element);
        if (uuid.isEmpty()) return "root";

        return uuid;
    }

    quintptr idFromElement_(const QDomElement& element) const
    {
        if (element.isNull()) return 0;

        auto key = elementKey_(element);

        if (!elementToId_.contains(key)) {
            auto id = nextId_++;
            elementToId_[key] = id;
            idToElement_[id] = element;
            return id;
        }

        return elementToId_[key];
    }

    int childElementCount_(const QDomElement& element) const
    {
        auto count = 0;
        auto child = element.firstChildElement();

        while (!child.isNull()) {
            ++count;
            child = child.nextSiblingElement();
        }

        return count;
    }

    QDomElement nthChildElement_(const QDomElement& element, int n) const
    {
        auto child = element.firstChildElement();

        for (auto i = 0; i < n && !child.isNull(); ++i)
            child = child.nextSiblingElement();

        return child;
    }

    int rowOfElement_(const QDomElement& element) const
    {
        if (element.isNull()) return -1;

        auto row = 0;
        auto sibling = element.previousSiblingElement();

        while (!sibling.isNull()) {
            ++row;
            sibling = sibling.previousSiblingElement();
        }

        return row;
    }

    QModelIndex indexFromElement_(const QDomElement& element) const
    {
        if (element.isNull() || element == dom_.documentElement()) return {};

        auto parent = element.parentNode().toElement();

        // Find this element's row among its siblings
        auto row = 0;
        auto sibling = parent.firstChildElement();
        while (!sibling.isNull()) {
            if (sibling == element) break;
            ++row;
            sibling = sibling.nextSiblingElement();
        }

        auto id = idFromElement_(element);
        return createIndex(row, 0, id);
    }

    bool isDescendantOf_(
        const QDomElement& ancestor,
        const QDomElement& element) const
    {
        if (ancestor.isNull() || element.isNull()) return false;

        auto current = element.parentNode().toElement();
        while (!current.isNull()) {
            if (current == ancestor) return true;
            current = current.parentNode().toElement();
        }

        return false;
    }
};

} // namespace Fernanda
