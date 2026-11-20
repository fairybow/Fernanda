/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
 *
 * <https://fernanda.fairybow.com>
 * <https://github.com/fairybow/Fernanda>
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <QAbstractButton>
#include <QColor>
#include <QEnterEvent>
#include <QEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QRectF>
#include <QString>
#include <QWidget>
#include <QtMinMax>

#include <Coco/Path.h>

#include "ui/UiConstants.h"
#include "ui/widgets/ButtonWash.h"
#include "ui/widgets/Glyph.h"

namespace Fernanda::Ui {

// The base for small square glyph buttons on the style's chrome: a Lucide
// glyph, tinted from the palette, centered on the style's flat tool-button
// panel (ButtonWash), such as the tab row's close and + buttons and the
// sidebar footer's. It holds what such buttons share and nothing that differs.
//
// What differs is which glyph to paint now: glyphPath() and glyphRole(), its
// tint. paintEvent asks for both on every repaint, so a glyph backed by live
// state (a page's pin) needs no cached flag, and a fixed glyph returns
// constants. One Glyph::Cache, keyed on path and tint, renders only when
// either changes.
//
// A checkable button, while checked, fills its panel with
// GLYPH_BUTTON_CHECKED_PANEL_ROLE (the accent) and draws its glyph in
// GLYPH_BUTTON_CHECKED_ICON_ROLE, with the hover and press wash over the fill.
//
// It only emits clicked(); the owner wires what a click means, as QTabBar
// handles close in the bar, not the button. Its size is fixed from the
// subclass's UiConstants knobs, not a style metric: PM_TabCloseIndicator runs
// tiny on some styles.
//
// Not for buttons off the style's chrome: ZoomButton_ floats over content,
// paints its own wash and a fixed tint, and shares only Glyph.h
class GlyphButton : public QAbstractButton
{
    Q_OBJECT

public:
    explicit GlyphButton(int extent, int iconExtent, QWidget* parent)
        : QAbstractButton(parent)
        , iconExtent_(iconExtent)
    {
        setup_(extent);
    }

protected:
    // The one axis of divergence: the glyph this button paints right now.
    // Called every paintEvent, so a subclass backed by mutable state returns it
    // live rather than caching a flag
    [[nodiscard]] virtual Coco::Path glyphPath() const = 0;

    // The glyph's tint, resolved against the widget's current group. Called
    // alongside glyphPath, so a live-state subclass keeps path and tint in
    // step
    [[nodiscard]] virtual QPalette::ColorRole glyphRole() const = 0;

    void enterEvent(QEnterEvent* event) override
    {
        QAbstractButton::enterEvent(event);
        update(); // show the wash on hover
    }

    void leaveEvent(QEvent* event) override
    {
        QAbstractButton::leaveEvent(event);
        update(); // drop the wash when the pointer leaves
    }

    void paintEvent([[maybe_unused]] QPaintEvent* event) override
    {
        QPainter painter(this);

        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

        // Checked: the accent, in the shape the style gives a tool button's
        // panel
        if (isChecked()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(palette().color(GLYPH_BUTTON_CHECKED_PANEL_ROLE));
            painter.drawRoundedRect(
                QRectF(rect()),
                STYLE_TOOL_BUTTON_RADIUS,
                STYLE_TOOL_BUTTON_RADIUS);
        }

        // A flat button's panel: nothing at rest, the style's wash on hover and
        // press
        ButtonWash::paint(painter, this, rect());

        // Glyph clamped to the button so an over-large constant fills it rather
        // than overflowing
        auto extent = qMin(iconExtent_, qMin(width(), height()));
        if (extent <= 0) {
            return;
        }

        auto role = isChecked() ? GLYPH_BUTTON_CHECKED_ICON_ROLE : glyphRole();
        auto pixmap = glyph_.pixmap(
            glyphPath(),
            extent,
            palette().color(role),
            devicePixelRatioF());

        auto x = (width() - extent) / 2;
        auto y = (height() - extent) / 2;
        painter.drawPixmap(x, y, pixmap);
    }

private:
    int iconExtent_;
    Glyph::Cache glyph_{};

    void setup_(int extent)
    {
        setAttribute(Qt::WA_Hover, true); // hover state for the wash
        setFocusPolicy(Qt::NoFocus);
        setCursor(Qt::ArrowCursor);
        setFixedSize(extent, extent);
    }
};

} // namespace Fernanda::Ui
