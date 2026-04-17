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
#include <QWebEngineView>
#include <QWidget>

#include "core/Debug.h"
#include "core/Time.h"
#include "ui/WidgetMask.h"

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

    static bool firstEverLoad() { return firstEverLoad_; }

protected:
    // TODO: Disabled context menu for now, may want a custom one later
    virtual void contextMenuEvent(QContextMenuEvent* event) override
    {
        event->accept();
    }

private:
    inline static bool firstEverLoad_ = true;
    WidgetMask* mask_ = new WidgetMask(this);

    void setup_()
    {
        mask_->activate(true);

        connect(this, &WebEngineView::loadStarted, this, [this] {
            mask_->activate(true);
        });

        connect(this, &WebEngineView::loadFinished, this, [this] {
            Time::onNextTick(this, [this] { mask_->deactivate(); });
        });

        connect(
            this,
            &WebEngineView::loadFinished,
            this,
            [] {
                if (firstEverLoad_) firstEverLoad_ = false;
            },
            Qt::SingleShotConnection);
    }
};

} // namespace Fernanda
