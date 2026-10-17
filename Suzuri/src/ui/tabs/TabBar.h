/*
 * Suzuri — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <QApplication>
#include <QMouseEvent>
#include <QPoint>
#include <QSize>
#include <QTabBar>
#include <QtMinMax>

#include <Coco/Debug.h>

#include "ui/UiConstants.h"

namespace Suzuri::Ui {

// A plain QTabBar with three behaviors:
//
//   - built-in horizontal reordering (free from QTabBar),
//   - detection of a vertical drag-off that emits detachDragged() as intent,
//   - shrink-then-scroll tab sizing.
//
// Sizing: every tab prefers MAX_TAB_WIDTH_ and compresses no smaller than
// MIN_TAB_WIDTH_ as more tabs are added; once even the floor won't fit, the bar
// overflows to scroll buttons. tabSizeHint reports the preferred width and
// minimumTabSizeHint the floor — QTabBar's own layout interpolates between
// them. The load-bearing piece is minimumSizeHint(): forcing its width to 0
// lets the enclosing layout squeeze the whole bar to nothing rather than widen
// the window, so overflow becomes scroll. That same squeeze is what lets a
// right-hand + button hug the tabs and then pin to the edge (TabPaneLeaf lays
// the button immediately after the bar).
//
// TabBar itself stays plain: it does not create close buttons or pin
// indicators. The owning TabPaneLeaf installs a per-tab close/unpin button on
// the close side via setTabButton, so setTabsClosable is deliberately OFF here
// — Qt's managed close button is all-or-nothing and can't carry pin state or be
// swapped per tab, so the leaf owns the button instead. No flags, no alert
// widgets.
//
// Detect-only for drag: the bar emits detachDragged, and TabPaneLeaf and
// TabPaneTree turn that into a QDrag with its drop zones. The bar never learns
// what a pane or a window is
class TabBar : public QTabBar
{
    Q_OBJECT

public:
    explicit TabBar(QWidget* parentTabPaneLeaf)
        : QTabBar(parentTabPaneLeaf)
    {
        setup_();
    }

    ~TabBar() override { TRACER; }

    // Yield horizontally. Returning width 0 lets the enclosing layout squeeze
    // the bar to nothing under pressure (see class note); the natural height is
    // kept. Without this the bar would set a floor on the row's width and push
    // the window wider instead of scrolling — and the right-hand + button would
    // not pin to the edge
    QSize minimumSizeHint() const override
    {
        return { 0, QTabBar::minimumSizeHint().height() };
    }

signals:
    // A tab was dragged clear of the bar vertically. Pure intent — the owning
    // leaf decides what happens
    void detachDragged(int index);

    // A tab was pressed with the left button, and is now the current one
    void tabPressed(int index);

protected:
    // Preferred tab width: a lone tab is comfortably wide, and tabs stay
    // uniform and compress together as the bar fills. The height is the tab's
    // as drawn plus the gap the style leaves above it (rowHeight_)
    QSize tabSizeHint(int index) const override
    {
        auto size = QTabBar::tabSizeHint(index);

        if (MAX_TAB_WIDTH >= 0) {
            size.setWidth(MAX_TAB_WIDTH);
        }

        size.setHeight(rowHeight_(size.height()));
        return size;
    }

    // Floor tab width: tabs shrink no further than this before the bar scrolls
    QSize minimumTabSizeHint(int index) const override
    {
        auto size = QTabBar::minimumTabSizeHint(index);

        if (MIN_TAB_WIDTH >= 0) {
            size.setWidth(MIN_TAB_WIDTH);
        }

        size.setHeight(rowHeight_(size.height()));
        return size;
    }

    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton) {
            dragStartPos_ = event->pos();
            dragPressIndex_ = tabAt(dragStartPos_);
        }

        QTabBar::mousePressEvent(event);

        // The base makes a left-pressed tab current, so it is by now
        if (event->button() == Qt::LeftButton && dragPressIndex_ > -1) {
            emit tabPressed(dragPressIndex_);
        }
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        // Let QTabBar handle horizontal reordering. Only a sufficiently
        // VERTICAL drag counts as a detach, so sideways motion still reorders
        // naturally
        if (dragPressIndex_ > -1 && (event->buttons() & Qt::LeftButton)) {
            auto delta = event->pos() - dragStartPos_;
            auto threshold = QApplication::startDragDistance() * 1.5;

            if (qAbs(delta.y()) >= threshold) {
                auto index = dragPressIndex_;
                dragPressIndex_ = -1; // one shot per press
                emit detachDragged(index);
                return;
            }
        }

        QTabBar::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
        dragPressIndex_ = -1;
        QTabBar::mouseReleaseEvent(event);
    }

private:
    QPoint dragStartPos_{};
    int dragPressIndex_ = -1;

    // A tab's full height: TAB_HEIGHT (or, at -1, the height Qt would give it)
    // plus STYLE_TAB_TOP_GAP, which the style leaves empty above the tab
    // (Style::drawTabShape_)
    [[nodiscard]] static int rowHeight_(int qtHeight)
    {
        return (TAB_HEIGHT >= 0 ? TAB_HEIGHT : qtHeight) + STYLE_TAB_TOP_GAP;
    }

    void setup_()
    {
        setMovable(true); // horizontal reorder, for free

        // No setTabsClosable(true): the leaf installs its own close/unpin
        // button per tab (see class note). Qt's managed close button is
        // all-or-nothing and can't carry pin state, so we own the button

        setExpanding(false);
        setDrawBase(false);
        setUsesScrollButtons(true);
        setElideMode(Qt::ElideRight); // titles truncate as tabs hit the floor

        // Tab marks (PageIcon) are drawn by the style ahead of the title.
        // Tab widths are fixed by tabSizeHint, so an icon never widens a tab —
        // it takes room from the title, which elides
        setIconSize({ TAB_ICON_EXTENT, TAB_ICON_EXTENT });

        // QTabBar reorders live mid-drag: each slide past a neighbor calls
        // moveTab and emits tabMoved, so an index captured at press goes stale.
        // Remap it through every move, exactly as QTabBar remaps its own
        // private pressedIndex (QTabBarPrivate::calculateNewPosition), so
        // detachDragged names the tab that was actually pressed
        connect(this, &QTabBar::tabMoved, this, [this](int from, int to) {
            if (dragPressIndex_ < 0) {
                return;
            }

            if (dragPressIndex_ == from) {
                dragPressIndex_ = to;
                return;
            }

            auto range_start = qMin(from, to);
            auto range_end = qMax(from, to);

            if (dragPressIndex_ >= range_start &&
                dragPressIndex_ <= range_end) {
                dragPressIndex_ += (from < to) ? -1 : 1;
            }
        });
    }
};

} // namespace Suzuri::Ui
