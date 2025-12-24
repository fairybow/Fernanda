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
#include <QIcon>
#include <QMimeData>
#include <QModelIndex>
#include <QModelIndexList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <Qt>
#include <QtTypes>

#include "Coco/Bool.h"
#include "Coco/Path.h"

#include "Debug.h"
#include "Fnx.h"
#include "FnxModelCache.h"

namespace Fernanda {

// See:
// https://doc.qt.io/qt-6/model-view-programming.html#model-subclassing-reference
//
// Qt Model/View adapter for .fnx virtual directory structure.
//
// Owns the internal QDomDocument. Public methods return FileInfo structs, never
// raw DOM elements. Uses Fnx::Xml helpers internally for DOM operations.
//
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

    explicit FnxModel(QObject* parent = nullptr)
        : QAbstractItemModel(parent)
    {
        setup_();
    }

    virtual ~FnxModel() override { TRACER; }

    void load(const Coco::Path& workingDir)
    {
        beginResetModel();
        dom_ = Fnx::Xml::makeDom(workingDir);
        domSnapshot_ = dom_.toString();
        cache_.clear();
        endResetModel();

        emit domChanged();
    }

    void write(const Coco::Path& workingDir) const
    {
        Fnx::Xml::writeModelFile(workingDir, dom_);
        INFO("DOM written: {}", dom_.toString());
    }

    void resetSnapshot() { domSnapshot_ = dom_.toString(); }

    bool isModified() const
    {
        // - QDomDocument::toString() is deterministic for the same structure
        // - Element/attribute order is preserved
        // - Whitespace handling is consistent
        return domSnapshot_ != dom_.toString();
    }

    QModelIndex notebookIndex() const
    {
        auto notebook = Fnx::Xml::notebookElement(dom_);
        return indexFromElement_(notebook);
    }

    QModelIndex trashIndex() const
    {
        auto trash = Fnx::Xml::trashElement(dom_);
        return indexFromElement_(trash);
    }

    bool hasTrash() const
    {
        return !Fnx::Xml::trashElement(dom_).firstChildElement().isNull();
    }

    void setFileEdited(const QString& uuid, bool edited)
    {
        auto element = findElementByUuid_(uuid);

        if (element.isNull()) {
            WARN("Cannot find element with UUID: {}", uuid);
            return;
        }

        if (!Fnx::Xml::isFile(element)) return;
        if (Fnx::Xml::isEdited(element) == edited) return;

        Fnx::Xml::setEdited(element, edited);
        emit domChanged();
    }

    FileInfo fileInfoAt(const QModelIndex& index) const
    {
        if (!index.isValid()) return {};
        return elementAt_(index);
    }

    // Parent/index is included
    QList<FileInfo> fileInfosAt(const QModelIndex& index) const
    {
        if (!index.isValid()) return {};

        auto element = elementAt_(index);
        if (element.isNull()) return {};

        QList<FileInfo> infos{};
        collectFileInfosRecursive_(element, infos);
        return infos;
    }

    void addNewVirtualFolder(const QModelIndex& parentIndex = {})
    {
        auto element = Fnx::Xml::addVirtualFolder(dom_);
        if (element.isNull()) return;

        auto parent = resolveParent_(parentIndex);
        insertElement_(element, parent);
    }

    FileInfo addNewTextFile(
        const Coco::Path& workingDir,
        const QModelIndex& parentIndex = {})
    {
        auto element = Fnx::Xml::addNewTextFile(workingDir, dom_);
        if (element.isNull()) return {};

        auto parent = resolveParent_(parentIndex);
        insertElement_(element, parent);

        return { element };
    }

    QList<FileInfo> importTextFiles(
        const Coco::Path& workingDir,
        const QList<Coco::Path>& fsPaths,
        const QModelIndex& parentIndex = {})
    {
        QList<QDomElement> elements{};

        for (const auto& fs_path : fsPaths) {
            if (!fs_path.exists()) continue;
            auto element = Fnx::Xml::importTextFile(workingDir, dom_, fs_path);
            if (!element.isNull()) elements << element;
        }

        if (elements.isEmpty()) return {};

        auto parent = resolveParent_(parentIndex);
        insertElements_(elements, parent);

        QList<FileInfo> infos{};

        for (const auto& element : elements)
            infos << element;

        return infos;
    }

    bool moveToTrash(const QModelIndex& index)
    {
        if (!index.isValid()) return false;

        auto element = elementAt_(index);
        if (element.isNull()) return false;

        // Store original parent's UUID for potential restore
        auto parent = element.parentNode().toElement();
        auto parent_uuid = Fnx::Xml::uuid(parent);
        if (!parent_uuid.isEmpty()) {
            Fnx::Xml::setRestoreParentUuid(element, parent_uuid);
        }

        auto trash = Fnx::Xml::trashElement(dom_);
        return moveElement_(element, trash, -1);
    }

    bool moveToNotebook_(const QModelIndex& index)
    {
        if (!index.isValid()) return false;

        auto element = elementAt_(index);
        if (element.isNull()) return false;

        // Try to find original parent
        auto old_parent_uuid = Fnx::Xml::restoreParentUuid(element);
        QDomElement destination = Fnx::Xml::notebookElement(dom_);

        if (!old_parent_uuid.isEmpty()) {
            auto old_parent = findElementByUuid_(old_parent_uuid);
            if (!old_parent.isNull()
                && !Fnx::Xml::isInTrash(dom_, old_parent)) {
                destination = old_parent;
            }
        }

        Fnx::Xml::clearRestoreParentUuid(element);
        return moveElement_(element, destination, -1);
    }

    bool remove(const QModelIndex& index)
    {
        if (!index.isValid()) return false;

        auto element = elementAt_(index);
        if (!isValid_(element)) {
            WARN("Removal attempted on invalid element!");
            return false;
        }

        auto parent_element = element.parentNode().toElement();
        if (parent_element.isNull()) return false;

        auto parent_index = indexFromElement_(parent_element);
        auto row = cache_.rowOf(element);

        // Purge cache BEFORE removing from DOM (need to traverse children)
        cache_.purgeSubtree(element);

        beginRemoveRows(parent_index, row, row);
        parent_element.removeChild(element);
        cache_.recordRemoval(parent_element, element);
        endRemoveRows();

        emit domChanged();
        return true;
    }

    bool clearTrash()
    {
        auto trash = Fnx::Xml::trashElement(dom_);
        if (!isValid_(trash)) {
            FATAL("Trash element is invalid! Something is extra wrong!");
        }

        auto child_count = cache_.childCount(trash);
        if (child_count <= 0) return true;

        // Purge cache for all children BEFORE removing from DOM
        auto child = trash.firstChildElement();
        while (!child.isNull()) {
            cache_.purgeSubtree(child);
            child = child.nextSiblingElement();
        }

        auto trash_index = trashIndex();
        beginRemoveRows(trash_index, 0, child_count - 1);

        // Remove all children from DOM
        while (!trash.firstChildElement().isNull()) {
            trash.removeChild(trash.firstChildElement());
        }

        cache_.invalidateChildren(trash);
        endRemoveRows();

        emit domChanged();
        return true;
    }

    // TODO: Unused
    int descendantCount(const QModelIndex& index) const
    {
        auto element = elementAt_(index);
        return descendantCountRecursive_(element);
    }

    // TODO: Unused
    int trashCount() const { return descendantCount(trashIndex()); }

    virtual Qt::ItemFlags flags(const QModelIndex& index) const override
    {
        auto default_flags = QAbstractItemModel::flags(index);

        if (!index.isValid()) {
            // Root can accept drops but isn't draggable/editable
            return default_flags | Qt::ItemIsDropEnabled;
        }

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
            if (new_name.isEmpty()) return false;

            Fnx::Xml::rename(element, new_name);

            emit dataChanged(index, index, { Qt::DisplayRole, Qt::EditRole });
            if (Fnx::Xml::isFile(element)) emit fileRenamed({ element });
            emit domChanged();

            return true;
        }

            // TODO: Handle other roles?

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

        auto child_element = cache_.childAt(parent_element, row);
        if (child_element.isNull()) return {};

        return createIndex(row, column, cache_.idOf(child_element));
    }

    virtual QModelIndex parent(const QModelIndex& child) const override
    {
        if (!child.isValid()) return {};

        auto child_element = elementAt_(child);
        if (child_element.isNull()) return {};

        auto parent_element = child_element.parentNode().toElement();

        if (parent_element.isNull()
            || parent_element == dom_.documentElement()) {
            return {};
        }

        return createIndex(
            cache_.rowOf(parent_element),
            0,
            cache_.idOf(parent_element));
    }

    virtual int rowCount(const QModelIndex& parent = {}) const override
    {
        if (parent.column() > 0) return 0;

        auto element = elementAt_(parent);
        if (element.isNull()) return 0;

        return cache_.childCount(element);
    }

    // TODO (maybe)
    virtual int columnCount(const QModelIndex& parent = {}) const override
    {
        (void)parent;
        return 1;
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

        QModelIndex index = indexes.first();
        if (!index.isValid()) return nullptr;

        auto element = elementAt_(index);
        if (element.isNull()) return nullptr;

        auto key = FnxModelCache::keyOf(element);
        auto mime_data = new QMimeData{};
        mime_data->setData(MIME_TYPE_, key.toUtf8());

        return mime_data;
    }

    virtual bool canDropMimeData(
        const QMimeData* data,
        Qt::DropAction action,
        int row,
        int column,
        const QModelIndex& parent) const override
    {
        (void)row;
        (void)column;
        (void)parent;

        if (!data || action != Qt::MoveAction) return false;
        return data->hasFormat(MIME_TYPE_);
    }

    virtual bool dropMimeData(
        const QMimeData* data,
        Qt::DropAction action,
        int row,
        int column,
        const QModelIndex& parent) override
    {
        (void)column;

        if (!data || action != Qt::MoveAction) return false;
        if (!data->hasFormat(MIME_TYPE_)) return false;

        auto key = QString::fromUtf8(data->data(MIME_TYPE_));
        auto element_id = cache_.idForKey(key);

        if (element_id == 0) {
            WARN("Drop: unknown element key: {}", key);
            return false;
        }

        auto element = cache_.elementAt(element_id);
        if (!isValid_(element)) {
            WARN("Drop: invalid element for ID: {}", element_id);
            cache_.clear(FnxModelCache::OnError::Yes);
            return false;
        }

        auto drop_parent = elementAt_(parent);
        if (drop_parent.isNull()) drop_parent = dom_.documentElement();

        return moveElement_(element, drop_parent, row);
    }

signals:
    void domChanged();
    void fileRenamed(const FileInfo& info);

private:
    COCO_BOOL(AllowOrphaned_);
    static constexpr auto MIME_TYPE_ = "application/x-fernanda-fnx-element";
    QDomDocument dom_{};
    QString domSnapshot_{};

    mutable FnxModelCache cache_{};
    mutable QIcon cachedDirIcon_{};
    mutable QIcon cachedFileIcon_{};

    void setup_()
    {
        //...
    }

    // All O(1) pointer operations. Should be cheap!
    bool isValid_(
        const QDomElement& element,
        AllowOrphaned_ allowOrphaned = AllowOrphaned_::No) const
    {
        if (element.isNull()) return false;

        if (allowOrphaned) return element.ownerDocument() == dom_;

        if (element.ownerDocument() != dom_) return false;
        if (element == dom_.documentElement()) return true;
        return !element.parentNode().toElement().isNull();
    }

    // Returns element for index. Invalid index returns document root
    QDomElement elementAt_(const QModelIndex& index) const
    {
        if (!index.isValid()) return dom_.documentElement();

        auto id = reinterpret_cast<quintptr>(index.internalPointer());
        return cache_.elementAt(id);
    }

    // Returns parent element, defaulting to document root if index invalid
    QDomElement resolveParent_(const QModelIndex& parentIndex) const
    {
        auto parent = elementAt_(parentIndex);
        return parent.isNull() ? dom_.documentElement() : parent;
    }

    // Creates QModelIndex for element. Returns invalid for null/root
    QModelIndex indexFromElement_(const QDomElement& element) const
    {
        if (element.isNull() || element == dom_.documentElement()) return {};

        auto row = cache_.rowOf(element);
        if (row < 0) return {};

        return createIndex(row, 0, cache_.idOf(element));
    }

    // Cache is lazily populated during tree traversal; unopened branches won't
    // be cached yet.
    QDomElement findElementByUuid_(const QString& uuid) const
    {
        if (uuid.isEmpty()) return {};

        // Check cache first
        if (cache_.containsKey(uuid)) {
            auto id = cache_.idForKey(uuid);
            return cache_.elementAt(id);
        }

        // Fall back to DOM search
        return findElementByUuidRecursive_(dom_.documentElement(), uuid);
    }

    QDomElement findElementByUuidRecursive_(
        const QDomElement& parent,
        const QString& uuid) const
    {
        auto child = parent.firstChildElement();

        while (!child.isNull()) {
            if (Fnx::Xml::uuid(child) == uuid) {
                cache_.cache(child);
                return child;
            }

            auto found = findElementByUuidRecursive_(child, uuid);
            if (!found.isNull()) return found;

            child = child.nextSiblingElement();
        }

        return {};
    }

    void collectFileInfosRecursive_(
        const QDomElement& element,
        QList<FileInfo>& outInfos) const
    {
        if (element.isNull()) return;

        if (Fnx::Xml::isFile(element)) outInfos << element;

        auto child = element.firstChildElement();
        while (!child.isNull()) {
            collectFileInfosRecursive_(child, outInfos);
            child = child.nextSiblingElement();
        }
    }

    // TODO: Unused
    int descendantCountRecursive_(const QDomElement& element) const
    {
        auto count = 0;
        auto child = element.firstChildElement();

        while (!child.isNull()) {
            ++count;
            count += descendantCountRecursive_(child);
            child = child.nextSiblingElement();
        }

        return count;
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
        // Both must be attached to tree
        if (!isValid_(element) || !isValid_(newParent)) {
            WARN("Move attempted on invalid element(s)!");
            return false;
        }

        auto current_parent = element.parentNode().toElement();
        if (current_parent.isNull()) return false;

        // Prevent moving into own subtree
        if (isDescendantOf_(element, newParent)) return false;

        auto src_parent_index = indexFromElement_(current_parent);
        auto source_row = cache_.rowOf(element);
        auto dest_parent_index = indexFromElement_(newParent);

        auto dest_child_count = cache_.childCount(newParent);
        auto dest_row = (newRow < 0) ? dest_child_count : newRow;

        // No-op check
        if (current_parent == newParent && source_row == dest_row) return true;

        INFO(
            "Moving element: {}\n\tFrom: {} row {}\n\tTo: {} row {}",
            element,
            current_parent,
            source_row,
            newParent,
            dest_row);

        // Calculate adjusted rows for Qt's beginMoveRows semantics
        auto dest_row_for_begin = dest_row;
        auto dest_row_for_dom = dest_row;

        if (current_parent == newParent && dest_row > source_row) {
            ++dest_row_for_begin; // Qt expects pre-removal index
            --dest_row_for_dom; // DOM needs post-removal index
        }

        if (!beginMoveRows(
                src_parent_index,
                source_row,
                source_row,
                dest_parent_index,
                dest_row_for_begin)) {
            return false;
        }

        // For same-parent moves, cache the sibling BEFORE any modifications
        // (indices shift after removal)
        QDomElement insert_before_sibling{};
        if (current_parent == newParent) {
            int sibling_index =
                (dest_row > source_row) ? dest_row : dest_row_for_dom;
            if (sibling_index < dest_child_count) {
                insert_before_sibling =
                    cache_.childAt(newParent, sibling_index);
            }
        } else {
            if (dest_row_for_dom < dest_child_count) {
                insert_before_sibling =
                    cache_.childAt(newParent, dest_row_for_dom);
            }
        }

        // DOM modification
        current_parent.removeChild(element);

        if (insert_before_sibling.isNull()) {
            newParent.appendChild(element);
        } else {
            newParent.insertBefore(element, insert_before_sibling);
        }

        // Cache update
        cache_.recordMove(current_parent, newParent, element, dest_row_for_dom);

        endMoveRows();
        emit domChanged();

        return true;
    }

    // The parent element parameters must be by-value because
    // `dom_.documentElement()` returns a temporary (rvalue). Non-const
    // references (`QDomElement&`) cannot bind to temporaries
    void insertElement_(const QDomElement& element, QDomElement parentElement)
    {
        if (!isValid_(element, AllowOrphaned_::Yes)
            || !isValid_(parentElement)) {
            WARN("Insertion attempted with invalid element(s)!");
            return;
        }

        auto parent_index = indexFromElement_(parentElement);
        auto row = cache_.childCount(parentElement);

        beginInsertRows(parent_index, row, row);

        parentElement.appendChild(element);
        cache_.recordInsertion(parentElement, element);
        endInsertRows();

        emit domChanged();
    }

    // The parent element parameters must be by-value because
    // `dom_.documentElement()` returns a temporary (rvalue). Non-const
    // references (`QDomElement&`) cannot bind to temporaries
    // TODO: Expand parent if applicable after appending (probably a view op)
    void insertElements_(
        const QList<QDomElement>& elements,
        QDomElement parentElement)
    {
        if (elements.isEmpty()) return;

        if (!isValid_(parentElement)) {
            WARN("Insertion attempted on invalid parent!");
            return;
        }

        auto parent_index = indexFromElement_(parentElement);
        auto row = cache_.childCount(parentElement);

        for (const auto& element : elements) {
            if (!isValid_(element, AllowOrphaned_::Yes)) {
                WARN("Insertion attempted with invalid element!");
                continue;
            }

            beginInsertRows(parent_index, row, row);
            parentElement.appendChild(element);
            cache_.recordInsertion(parentElement, element);
            endInsertRows();
            ++row;
        }

        emit domChanged();
    }
};

} // namespace Fernanda

// Tests:

/*#include <QElapsedTimer>
#include <atomic>

namespace FnxModelProfile {

inline std::atomic<int> indexCalls{ 0 };
inline std::atomic<int> parentCalls{ 0 };
inline std::atomic<int> rowCountCalls{ 0 };
inline std::atomic<int> dataCalls{ 0 };
inline std::atomic<int> isValidCalls{ 0 };

inline void report()
{
    DEBUG("=== FnxModel Call Counts ===");
    DEBUG("index(): {}", indexCalls.load());
    DEBUG("parent(): {}", parentCalls.load());
    DEBUG("rowCount(): {}", rowCountCalls.load());
    DEBUG("data(): {}", dataCalls.load());
    DEBUG("isValid_(): {}", isValidCalls.load());
    indexCalls = parentCalls = rowCountCalls = dataCalls = isValidCalls = 0;
}

} // namespace FnxModelProfile*/
