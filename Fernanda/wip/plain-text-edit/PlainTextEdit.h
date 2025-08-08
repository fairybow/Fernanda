#pragma once

#include <QKeyEvent>
#include <QObject>
#include <QPlainTextEdit>
#include <QWidget>

#include "Coco/Debug.h"

#include "KeyFilter.h"

class PlainTextEdit : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit PlainTextEdit(QWidget* parent = nullptr)
        : QPlainTextEdit(parent)
    {
        initialize_();
    }

    virtual ~PlainTextEdit() override { COCO_TRACER; }

protected:
    virtual void keyPressEvent(QKeyEvent* event) override
    {
        if (keyFilter_ && keyFilter_->handle(this, event)) return;
        QPlainTextEdit::keyPressEvent(event);
    }

private:
    KeyFilter* keyFilter_ = new KeyFilter(this);

    void initialize_()
    {
        //...
    }
};
