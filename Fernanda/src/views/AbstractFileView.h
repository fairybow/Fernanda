/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
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

#include "models/AbstractFileModel.h"
#include "core/Debug.h"

namespace Fernanda {

// Abstract UI interface for file content display and editing, providing common
// view operations (cut/copy/paste/select) and view state management without
// business logic.
//
// Calling a pure virtual (`setupWidget()`) from a base class constructor would
// dispatch to the base, not the derived class, since the derived class is not
// yet constructed. Two-phase initialization avoids this: the object is fully
// constructed first, then `initialize()` is called from outside, at which point
// the virtual call resolves correctly
class AbstractFileView : public QWidget
{
    Q_OBJECT

public:
    explicit AbstractFileView(
        AbstractFileModel* fileModel,
        QWidget* parent = nullptr)
        : QWidget(parent)
        , fileModel_(fileModel)
    {
        if (!fileModel) FATAL("AbstractFileModel cannot be nullptr!");
        layout_ = new QVBoxLayout(this);
        layout_->setContentsMargins(0, 0, 0, 0);
        layout_->setSpacing(0);
    }

    virtual ~AbstractFileView() = default;

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

    // QWidget* widget() const noexcept { return widget_; }
    AbstractFileModel* model() const noexcept { return fileModel_; }

    /// TODO FT: Re-examine this as the contract for this abstract base. What
    /// should be the contract?
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
