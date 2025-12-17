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
// Owns the internal QDomDocument. Public methods return FileInfo structs, never
// raw DOM elements. Uses Fnx::Xml helpers internally for DOM operations.
//
// TODO: Trash (should be immutable and separate from active)
// TODO: Double clicking on files should maybe not expand (if they have
// children), since they also open with double clicks?
class FnxModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    struct FileInfo
    {
        Coco::Path relPath{};
        QString name{};

        FileInfo(const QDomElement& element)
            : relPath(Fnx::Xml::relPath(element))
            , name(Fnx::Xml::name(element))
        {
        }

        FileInfo() = default;
        // TODO: Could take workingDir param and concat path and check exists?
        bool isValid() const { return !relPath.isEmpty() && !name.isEmpty(); }
    };

    FnxModel(QObject* parent)
        : QAbstractItemModel(parent)
    {
    }

    virtual ~FnxModel() override { TRACER; }

    // TODO: Is this the right approach?
    QModelIndex notebookIndex() const
    {
        auto notebook = Fnx::Xml::notebookElement(dom_);
        return indexFromElement_(notebook);
    }

    // TODO: Is this the right approach?
    QModelIndex trashIndex() const
    {
        auto trash = Fnx::Xml::trashElement(dom_);
        return indexFromElement_(trash);
    }

    void load(const Coco::Path& workingDir)
    {
        beginResetModel();
        dom_ = Fnx::Xml::makeDom(workingDir);
        domSnapshot_ = dom_.toString();
        clearCache_();
        endResetModel();

        emit domChanged();
    }

    void write(const Coco::Path& workingDir) const
    {
        Fnx::Xml::writeModelFile(workingDir, dom_);
        INFO("DOM written: {}", dom_.toString());
    }

    void resetSnapshot() { domSnapshot_ = dom_.toString(); }

    // TODO: Problem with this method if we ever decide to store
    // expanded/collapsed state in the DOM...
    bool isModified() const
    {
        // - QDomDocument::toString() is deterministic for the same structure
        // - Element/attribute order is preserved
        // - Whitespace handling is consistent
        return domSnapshot_ != dom_.toString();
    }

    void setFileEdited(const QString& uuid, bool edited)
    {
        QDomElement element{};

        // Try cache first
        if (elementToId_.contains(uuid)) {
            auto id = elementToId_[uuid];
            element = idToElement_[id];
        } else {
            // Fallback: search DOM directly
            element = findElementByUuid_(uuid);

            if (element.isNull()) {
                WARN("Cannot find element with UUID: {}", uuid);
                return;
            }

            // Populate cache now that we found it
            idFromElement_(element);
        }

        if (!Fnx::Xml::isFile(element)) return;

        // Avoid unnecessary domChanged emissions
        if (Fnx::Xml::isEdited(element) == edited) return;

        Fnx::Xml::setEdited(element, edited);
        emit domChanged();
    }

    FileInfo fileInfoAt(const QModelIndex& index) const
    {
        if (!index.isValid()) return {};
        return { elementAt_(index) };
    }

    void addNewVirtualFolder(const QModelIndex& parentIndex = {})
    {
        auto element = Fnx::Xml::addVirtualFolder(dom_);
        if (element.isNull()) return;

        // TODO: Code duplicated below, in addNewTextFile and (partially)
        // importTextFiles
        auto parent = elementAt_(parentIndex);
        if (parent.isNull()) parent = dom_.documentElement();
        insertElement_(element, parent);
    }

    FileInfo addNewTextFile(
        const Coco::Path& workingDir,
        const QModelIndex& parentIndex = {})
    {
        // Use FnxModel's internal DOM
        auto element = Fnx::Xml::addNewTextFile(workingDir, dom_);
        if (element.isNull()) return {};

        // Insert the element
        auto parent = elementAt_(parentIndex);
        if (parent.isNull()) parent = dom_.documentElement();
        insertElement_(element, parent);

        // Return only the non-DOM parts
        return { element };
    }

    QList<FileInfo> importTextFiles(
        const Coco::Path& workingDir,
        const QList<Coco::Path>& fsPaths,
        const QModelIndex& parentIndex = {})
    {
        QList<QDomElement> elements{};

        // Create all physical files and elements
        for (const auto& fs_path : fsPaths) {
            if (!fs_path.exists()) continue;
            auto element = Fnx::Xml::importTextFile(workingDir, dom_, fs_path);
            if (element.isNull()) continue;
            elements << element;
        }

        if (elements.isEmpty()) return {};

        auto parent = elementAt_(parentIndex);
        if (parent.isNull()) parent = dom_.documentElement();
        insertElements_(elements, parent); // Single domChanged emission

        QList<FileInfo> infos{};
        for (auto& element : elements)
            infos << FileInfo{ element };

        return infos;
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

        auto element = elementAt_(index);
        if (element.isNull()) return false;

        switch (role) {
        case Qt::EditRole:
        case Qt::DisplayRole: {
            // Handle rename
            auto new_name = value.toString();
            if (new_name.isEmpty()) return false; // Reject empty names

            // Renames for files and virtual folders but only notifies for files
            // specifically plus the full DOM in general
            Fnx::Xml::rename(element, new_name);

            emit dataChanged(index, index, { Qt::DisplayRole, Qt::EditRole });
            if (Fnx::Xml::isFile(element)) emit fileRenamed({ element });
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

        auto parent_element = elementAt_(parent);
        if (parent_element.isNull()) return {};
        auto child_element = nthChildElement_(parent_element, row);
        if (child_element.isNull()) return {};

        return createIndex(row, column, idFromElement_(child_element));
    }

    virtual QModelIndex parent(const QModelIndex& child) const override
    {
        if (!child.isValid()) return {};

        auto child_element = elementAt_(child);
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
        auto element = elementAt_(parent);
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

        auto element = elementAt_(index);
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

        if (!isValid_(element)) {
            WARN("Invalid element in drag data for key: {}", key.toStdString());
            clearCache_();
            return false;
        }

        // Determine drop target
        auto drop_parent = elementAt_(parent);
        if (drop_parent.isNull()) drop_parent = dom_.documentElement();

        return moveElement_(element, drop_parent, row);
    }

signals:
    void domChanged();
    void fileRenamed(const FileInfo& info);

private:
    static constexpr auto MIME_TYPE_ = "application/x-fernanda-fnx-element";
    QDomDocument dom_{};
    QString domSnapshot_{};

    // ID allocation: 0 = invalid/root element
    // IDs start at 1 and increment monotonically
    // IDs are stable across DOM modifications when using UUID-based tracking
    mutable quintptr nextId_ = 1;
    mutable QHash<QString, quintptr> elementToId_{}; // Element's UUID -> ID
    mutable QHash<quintptr, QDomElement> idToElement_{}; // ID -> Element

    void clearCache_() const
    {
        elementToId_.clear();
        idToElement_.clear();
        nextId_ = 1;
        INFO("DOM cache cleared!");
    }

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

    bool isValidForInsertion_(const QDomElement& element) const
    {
        if (element.isNull()) return false;
        // Only check document ownership, not parent
        return element.ownerDocument() == dom_;
    }

    QDomElement elementAt_(const QModelIndex& index) const
    {
        if (!index.isValid()) return dom_.documentElement();

        auto id = reinterpret_cast<quintptr>(index.internalPointer());
        auto element = idToElement_.value(id);

        if (!isValid_(element)) {
            WARN("Invalid element for ID: {}", id);
            clearCache_();
            return {};
        }

        return element;
    }

    // TODO: Fix
    QString elementKey_(const QDomElement& element) const
    {
        if (element.isNull()) return {};

        auto uuid = Fnx::Xml::uuid(element);
        if (!uuid.isEmpty()) return uuid;

        // Structural elements without UUIDs, use tag name as key
        // TODO: Potentially just return tag name if UUID is empty?
        auto tag = element.tagName();
        if (tag == "fnx" || tag == "notebook" || tag == "trash") return tag;

        // TODO: Fatal, maybe

        return "root"; // Fallback (shouldn't happen)
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

    // Cache is lazily populated during tree traversal; unopened branches won't
    // be cached yet.
    QDomElement findElementByUuid_(const QString& uuid) const
    {
        if (uuid.isEmpty()) return {};
        return findElementByUuidRecursive_(dom_.documentElement(), uuid);
    }

    QDomElement findElementByUuidRecursive_(
        const QDomElement& parent,
        const QString& uuid) const
    {
        auto child = parent.firstChildElement();

        while (!child.isNull()) {
            if (Fnx::Xml::uuid(child) == uuid) return child;

            // Search children
            auto found = findElementByUuidRecursive_(child, uuid);
            if (!found.isNull()) return found;

            child = child.nextSiblingElement();
        }

        return {};
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

    // The parent element parameters must be by-value because
    // `dom_.documentElement()` returns a temporary (rvalue). Non-const
    // references (`QDomElement&`) cannot bind to temporaries
    bool
    moveElement_(const QDomElement& element, QDomElement newParent, int newRow)
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
        if (dest_row < 0) dest_row = childElementCount_(newParent);

        // Check move isn't pointless
        if (current_parent == newParent && source_row == dest_row) return true;

        INFO(
            "Moving element: {}\n\tOld parent: {}\n\tOld row: {}\n\tNew "
            "parent: {}\n\tNew row: {}",
            element,
            current_parent,
            source_row,
            newParent,
            dest_row);

        // Adjust destination row for beginMoveRows when moving within same
        // parent
        auto dest_row_for_begin = dest_row;
        auto dest_row_for_dom = dest_row;

        if (current_parent == newParent && dest_row > source_row) {
            ++dest_row_for_begin; // For beginMoveRows (pre-removal state)
            --dest_row_for_dom; // For DOM manipulation (post-removal state)
        }

        if (!beginMoveRows(
                source_parent_index,
                source_row,
                source_row,
                dest_parent_index,
                dest_row_for_begin)) {
            return false;
        }

        // Perform DOM manipulation
        current_parent.removeChild(element);

        if (dest_row_for_dom >= childElementCount_(newParent)) {
            newParent.appendChild(element);
        } else {
            auto sibling = nthChildElement_(newParent, dest_row_for_dom);
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

    // TODO: moveElements_ (maybe)

    // The parent element parameters must be by-value because
    // `dom_.documentElement()` returns a temporary (rvalue). Non-const
    // references (`QDomElement&`) cannot bind to temporaries
    // TODO: Could test QDomElement& again (here and elsewhere) once new tab is
    // fixed
    void insertElement_(const QDomElement& element, QDomElement parentElement)
    {
        if (!isValidForInsertion_(element) || !isValid_(parentElement)) return;

        auto parent_index = indexFromElement_(parentElement);
        auto row = childElementCount_(parentElement);

        beginInsertRows(parent_index, row, row);
        parentElement.appendChild(element);
        endInsertRows();

        emit domChanged();
    }

    // TODO: Expand parent if applicable after appending (probably a view op)
    // The parent element parameters must be by-value because
    // `dom_.documentElement()` returns a temporary (rvalue). Non-const
    // references (`QDomElement&`) cannot bind to temporaries
    void insertElements_(
        const QList<QDomElement>& elements,
        QDomElement parentElement)
    {
        if (elements.isEmpty() || !isValid_(parentElement)) return;

        auto parent_index = indexFromElement_(parentElement);
        auto row = childElementCount_(parentElement);

        for (const auto& element : elements) {
            if (!isValidForInsertion_(element)) continue;

            beginInsertRows(parent_index, row, row);
            parentElement.appendChild(element);
            endInsertRows();
            ++row;
        }

        emit domChanged();
    }
};

} // namespace Fernanda
