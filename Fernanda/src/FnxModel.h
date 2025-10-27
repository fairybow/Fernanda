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
#include <QtTypes>

#include "Debug.h"
#include "Fnx.h"

namespace Fernanda {

// See:
// https://doc.qt.io/qt-6/model-view-programming.html#model-subclassing-reference
// TODO: Trash (should be immutable and separate from active)
class FnxModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    FnxModel(QObject* parent)
        : QAbstractItemModel(parent)
    {
    }

    virtual ~FnxModel() override { TRACER; }

    QDomDocument domDocument() const { return dom_; }

    void setDomDocument(const QDomDocument& dom)
    {
        beginResetModel();
        dom_ = dom;
        elementToId_.clear();
        idToElement_.clear();
        nextId_ = 1; // Reserve 0 for root
        endResetModel();
    }

    virtual QModelIndex
    index(int row, int column, const QModelIndex& parent = {}) const override
    {
        if (!hasIndex(row, column, parent)) return {};

        auto parent_element = elementFromIndex_(parent);
        auto child_element = nthChildElement_(parent_element, row);

        if (child_element.isNull()) return {};

        return createIndex(row, column, idFromElement_(child_element));
    }

    virtual QModelIndex parent(const QModelIndex& child) const override
    {
        if (!child.isValid()) return {};

        auto child_element = elementFromIndex_(child);
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

        auto element = elementFromIndex_(parent);
        return childElementCount_(element);
    }

    virtual int columnCount(const QModelIndex& parent = {}) const override
    {
        return 1; // Just "Name" column for now
    }

    virtual QVariant
    data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

private:
    QDomDocument dom_{};
    mutable QHash<QString, quintptr> elementToId_{}; // Element -> unique ID
    mutable QHash<quintptr, QDomElement> idToElement_{}; // ID -> Element
    mutable quintptr nextId_ = 1;

    // Generate unique key for an element
    QString elementKey_(const QDomElement& element) const
    {
        if (element.isNull()) return {};

        // Build path like "root/0/2/1" for uniqueness
        QStringList path{};
        QDomElement current = element;

        while (!current.isNull() && current != dom_.documentElement()) {
            path.prepend(QString::number(rowOfElement_(current)));
            current = current.parentNode().toElement();
        }

        return path.join('/');
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

    QDomElement elementFromIndex_(const QModelIndex& index) const
    {
        if (!index.isValid()) return dom_.documentElement();

        auto id = reinterpret_cast<quintptr>(index.internalPointer());
        return idToElement_.value(id);
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
};

} // namespace Fernanda
