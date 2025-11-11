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
// Qt Model/View adapter for .fnx virtual directory structure.
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

    // TODO: For now (perhaps forever), we only set the DOM once
    void initialize(const QDomDocument& dom)
    {
        if (initialized_) return;
        dom_ = dom;
        initialized_ = true;
    }

    QDomDocument domDocument() const { return dom_; }

    // QDomElement acts as a handle to shared DOM data.
    // Passing by value is cheap (like copying a pointer).
    // Modifications through any copy affect the shared DOM.
    // Parameter is non-const to document that DOM will be modified.
    // TODO: Expand parent if applicable after appending (probably a view op)
    void insertElement(const QDomElement& element, QDomElement parentElement)
    {
        if (!isValid_(element) || !isValid_(parentElement)) return;

        auto parent_index = indexFromElement_(parentElement);
        auto row = childElementCount_(parentElement);

        beginInsertRows(parent_index, row, row);
        parentElement.appendChild(element);
        endInsertRows();

        emit domChanged();
    }

    // TODO: Expand parent if applicable after appending (probably a view op)
    void insertElements(
        const QList<QDomElement>& elements,
        QDomElement parentElement)
    {
        if (elements.isEmpty() || !isValid_(parentElement)) return;

        auto parent_index = indexFromElement_(parentElement);
        auto row = childElementCount_(parentElement);

        for (const auto& element : elements) {
            if (!isValid_(element)) continue;

            beginInsertRows(parent_index, row, row);
            parentElement.appendChild(element);
            endInsertRows();
            ++row;
        }

        emit domChanged();
    }

    // TODO: Add logs to document movement in case we encounter
    // QPersistentModelIndex crash again
    bool
    moveElement(const QDomElement& element, QDomElement newParent, int newRow)
    {
        if (!isValid_(element) || !isValid_(newParent)) {
            WARN("Move attempted on invalid element(s)");
            return false;
        }

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

        // Check move isn't pointless
        if (current_parent == newParent && source_row == dest_row) {
            return true;
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
    }

    // TODO: moveElements (maybe)

    QDomElement elementAt(const QModelIndex& index) const
    {
        if (!index.isValid()) return dom_.documentElement();

        auto id = reinterpret_cast<quintptr>(index.internalPointer());
        auto element = idToElement_.value(id);

        if (!isValid_(element)) {
            WARN("Stale/invalid element for ID {}", id);
            return {};
        }

        return element;
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
        if (parent_element.isNull()) return {};
        auto child_element = nthChildElement_(parent_element, row);
        if (child_element.isNull()) return {};

        return createIndex(row, column, idFromElement_(child_element));
    }

    virtual QModelIndex parent(const QModelIndex& child) const override
    {
        if (!child.isValid()) return {};

        auto child_element = elementAt(child);
        if (child_element.isNull()) return {};
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
        if (element.isNull()) return 0;

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

        // Store the element's key
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

        if (!isValid_(element)) return false;

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

    // ID allocation: 0 = invalid/root element
    // IDs start at 1 and increment monotonically
    // IDs are stable across DOM modifications when using UUID-based tracking
    mutable quintptr nextId_ = 1;
    mutable QHash<QString, quintptr> elementToId_{}; // Element's UUID -> ID
    mutable QHash<quintptr, QDomElement> idToElement_{}; // ID -> Element

    bool isValid_(const QDomElement& element) const
    {
        if (element.isNull()) return false;

        // Must belong to current document
        if (element.ownerDocument() != dom_) return false;

        // Root element is always valid
        if (element == dom_.documentElement()) return true;

        // Non-root elements must have an element parent (if orphaned, parent is
        // null)
        return !element.parentNode().toElement().isNull();
    }

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
        if (element.isNull()) return 0;

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
        if (element.isNull()) return {};

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

        auto row = rowOfElement_(element);
        if (row < 0) return {};

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
