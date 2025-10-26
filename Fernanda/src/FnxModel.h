#pragma once

#include <QAbstractItemModel>
#include <QDomDocument>
#include <QModelIndex>
#include <QObject>
#include <QVariant>
#include <Qt>

#include "Debug.h"
#include "Fnx.h"

namespace Fernanda {

// See:
// https://doc.qt.io/qt-6/model-view-programming.html#model-subclassing-reference
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
    void setDomDocument(const QDomDocument& dom) { dom_ = dom; }

    virtual QModelIndex
    index(int row, int column, const QModelIndex& parent = {}) const override
    {
        return {};
    }

    virtual QModelIndex parent(const QModelIndex& index) const override
    {
        return {};
    }

    virtual int rowCount(const QModelIndex& parent = {}) const override
    {
        return {};
    }

    virtual int columnCount(const QModelIndex& parent = {}) const override
    {
        return {};
    }

    virtual QVariant
    data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        return {};
    }

private:
    QDomDocument dom_{};
};

} // namespace Fernanda
