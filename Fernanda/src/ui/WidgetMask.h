/*
 * Fernanda — a plain-text-first workbench for creative writing
 * Copyright (C) 2025-2026 fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <QEvent>
#include <QObject>
#include <QWidget>

#include "core/Debug.h"
#include "core/Time.h"

namespace Fernanda {

class WidgetMask : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetMask(QWidget* parent, int resizeDebounceMs = 300)
        : QWidget(parent)
        , resizeDebouncer_(
              Time::newDebouncer(
                  this,
                  &WidgetMask::onResizeSettled_,
                  resizeDebounceMs))
    {
        ASSERT(parent);
        setAutoFillBackground(true);
        hide();
        parent->installEventFilter(this);
    }

    virtual ~WidgetMask() override { TRACER; }

    void activate(bool hold = false)
    {
        if (hold) held_ = true;

        syncToParent_();
        raise();
        show();
    }

    void deactivate(bool deleteAfter = false)
    {
        held_ = false;
        hide();
        if (deleteAfter) deleteLater();
    }

protected:
    virtual bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (watched == parent() && event->type() == QEvent::Resize) {
            activate();
            resizeDebouncer_->start();
        }

        return false;
    }

private:
    bool held_ = false;
    Time::Debouncer* resizeDebouncer_;

    void syncToParent_() { setFixedSize(parentWidget()->size()); }

    void onResizeSettled_()
    {
        if (!held_) deactivate();
    }
};

} // namespace Fernanda
