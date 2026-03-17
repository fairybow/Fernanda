/*
 * Fernanda is a plain text editor for fiction writing
 * Copyright (C) 2025-2026, fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <QObject>
#include <QSize>
#include <QStyle>
#include <QTabBar>
#include <QWidget>
#include <QtMinMax>

#include "core/Debug.h"

namespace Fernanda {

class TabWidgetTabBar : public QTabBar
{
    Q_OBJECT

public:
    explicit TabWidgetTabBar(QWidget* parent = nullptr)
        : QTabBar(parent)
    {
        setup_();
    }

    virtual ~TabWidgetTabBar() override { TRACER; }

    virtual QSize minimumSizeHint() const override { return { 0, height() }; }

    virtual QSize sizeHint() const override
    {
        auto hint = QTabBar::sizeHint();
        return { hint.width(), height() };
    }

    int minimumTabWidth() const noexcept { return minimumTabWidth_; }
    int maximumTabWidth() const noexcept { return maximumTabWidth_; }
    QSize minimumTabSize() const { return { minimumTabWidth_, height() }; }
    QSize maximumTabSize() const { return { maximumTabWidth_, height() }; }

    void setMinimumTabWidth(int width)
    {
        minimumTabWidth_ = qMax(0, qMin(width, maximumTabWidth_));
    }

    void setMaximumTabWidth(int width)
    {
        maximumTabWidth_ = qMax(qMax(0, width), minimumTabWidth_);
    }

protected:
    virtual QSize minimumTabSizeHint(int index) const override
    {
        return { minimumTabWidth_, height() };
    }

    virtual QSize tabSizeHint(int index) const override
    {
        return { maximumTabWidth_, height() };
    }

private:
    int minimumTabWidth_ = 75;
    int maximumTabWidth_ = 75;

    void setup_()
    {
        setMovable(true);
        setAutoHide(false);
        setTabsClosable(false); // Using custom buttons
        // TODO: Move close buttons here?

        // Should have no effect regardless, given our size hints
        setExpanding(false);
        setDrawBase(false);

        // TODO: Custom scrollers!
    }
};

} // namespace Fernanda
