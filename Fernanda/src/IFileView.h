/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QObject>
#include <QVBoxLayout>
#include <QWidget>

#include "Coco/Layout.h"

#include "AbstractFileModel.h"
#include "Debug.h"

namespace Fernanda {

// Abstract UI interface for file content display and editing, providing common
// view operations (cut/copy/paste/select) and view state management without
// business logic
class IFileView : public QWidget
{
    Q_OBJECT

public:
    explicit IFileView(AbstractFileModel* fileModel, QWidget* parent = nullptr)
        : QWidget(parent)
        , fileModel_(fileModel)
    {
        if (!fileModel) FATAL("AbstractFileModel cannot be nullptr!");
        layout_ = Coco::Layout::makeDense<QVBoxLayout*>(this);
    }

    virtual ~IFileView() = default;

    // Warning! Each view must be initialized after creation.
    void initialize()
    {
        if (initialized_) return;

        widget_ = setupWidget();

        if (widget_) {
            layout_->addWidget(widget_);
            setFocusProxy(widget_);
            initialized_ = true;
        }
    }

    AbstractFileModel* model() const noexcept { return fileModel_; }

    virtual bool supportsEditing() const = 0;

    virtual bool hasPaste() const { return false; }
    virtual bool hasSelection() const { return false; }
    virtual void cut() {}
    virtual void copy() {}
    virtual void paste() {}
    virtual void deleteSelection() {}
    virtual void selectAll() {}

signals:
    void selectionChanged();
    void clipboardDataChanged();

protected:
    // For subclasses to setup their widget, called here (from the base) in
    // initialize (which must be called from outside after creating the view)
    virtual QWidget* setupWidget() = 0;

private:
    AbstractFileModel* fileModel_;

    bool initialized_ = false;
    QVBoxLayout* layout_ = nullptr;
    QWidget* widget_ = nullptr;
};

} // namespace Fernanda
