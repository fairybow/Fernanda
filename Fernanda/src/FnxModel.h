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
#include <QModelIndex>
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
        //beginResetModel();
        dom_ = dom;
        //elementToId_.clear();
        //idToElement_.clear();
        //nextId_ = 1; // Reserve 0 for root
        //endResetModel();
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

    // TODO: moveElement
    // TODO: moveElements (maybe)

    //void updateElement(const QDomElement& element)
    //{
    //    if (element.isNull()) return;
    //    auto index = indexFromElement_(element);
    //    if (!index.isValid()) return;
    //    // QAbstractItemModel::dataChanged in automatically connected when you
    //    // set a view's model
    //    emit dataChanged(index, index);
    //    emit domChanged();
    //}

    QDomElement elementAt(const QModelIndex& index) const
    {
        if (!index.isValid()) return dom_.documentElement();
        auto id = reinterpret_cast<quintptr>(index.internalPointer());
        return idToElement_.value(id);
    }

    // TODO: Don't trigger line edit on double click. Rather, use the staggered
    // click that is more common.
    virtual Qt::ItemFlags flags(const QModelIndex& index) const override
    {
        if (!index.isValid()) return Qt::NoItemFlags;
        return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
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

signals:
    void domChanged();
    void elementRenamed(const QDomElement& element);

private:
    bool initialized_ = false;
    QDomDocument dom_{};
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
            count++;
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
            row++;
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
};

} // namespace Fernanda
