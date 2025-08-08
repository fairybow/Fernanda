#pragma once

#include <QVBoxLayout>
#include <QWidget>

#include "Coco/Concepts.h"
#include "Coco/Debug.h"
#include "Coco/Layout.h"

#include "IFile.h"

class IFileView : public QWidget
{
    Q_OBJECT

public:
    explicit IFileView(IFile* model, QWidget* parent = nullptr)
        : QWidget(parent), model_(model)
    {
        layout_ = Coco::Layout::makeDense<QVBoxLayout*>(this);
    }

    virtual ~IFileView() override { COCO_TRACER; }

    // Must be called from outside after creating the view
    void initialize()
    {
        if (initialized_) return;

        widget_ = setupWidget();

        if (widget_)
        {
            layout_->addWidget(widget_);
            setFocusProxy(widget_);
            initialized_ = true;
        }
    }

    IFile* model() const noexcept { return model_; }

    template <IFilePointer T>
    T model() const { return qobject_cast<T>(model_); }

    QWidget* widget() const noexcept { return widget_; }

    template <Coco::Concepts::QWidgetPointer T>
    T widget() const { return qobject_cast<T>(widget_); }

private:
    IFile* model_;

    QVBoxLayout* layout_ = nullptr;
    bool initialized_ = false;
    QWidget* widget_ = nullptr;

    /// ======================================================================== ///
    /// *** FOR SUBCLASSES ***                                                   ///
    /// ======================================================================== ///

public:
    virtual bool canSelect() const = 0;
    virtual bool canPaste() const = 0;

    // For editable subclasses to override
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
};

template<typename T>
concept IFileViewPointer = Coco::Concepts::DerivedPointer<IFileView, T>;
