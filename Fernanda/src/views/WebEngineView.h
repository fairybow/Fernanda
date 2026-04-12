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

#include <QContextMenuEvent>
#include <QResizeEvent>
#include <QSize>
#include <QWebEngineView>
#include <QWidget>

#include "core/Debug.h"
#include "core/Time.h"

namespace Fernanda {

class WebEngineView : public QWebEngineView
{
    Q_OBJECT

public:
    explicit WebEngineView(QWidget* parent = nullptr)
        : QWebEngineView(parent)
    {
        setup_();
    }

    virtual ~WebEngineView() override { TRACER; }

protected:
    // TODO: Disabled context menu for now, may want a custom one later
    virtual void contextMenuEvent(QContextMenuEvent* event) override
    {
        event->accept();
    }

    virtual void resizeEvent(QResizeEvent* event) override
    {
        // Hide preview resize visual stutter and debounce
        showMask_();
        resizeDebouncer_->start();

        QWidget::resizeEvent(event);
    }

private:
    QWidget* mask_ = new QWidget(this);
    Time::Debouncer* resizeDebouncer_ =
        Time::newDebouncer(this, [this] { mask_->hide(); }, 300);

    void setup_()
    {
        mask_->setAutoFillBackground(true);
        mask_->hide();

        showMask_();

        connect(this, &WebEngineView::loadStarted, this, [this] {
            showMask_();
        });

        connect(this, &WebEngineView::loadFinished, this, [this] {
            Time::onNextTick(this, [this] { mask_->hide(); });
        });
    }

    void showMask_()
    {
        mask_->setFixedSize(size());
        mask_->raise();
        mask_->show();
    }
};

} // namespace Fernanda
