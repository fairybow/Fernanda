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

// The shared base for small square glyph buttons that sit on the style's own
// chrome: a Lucide glyph, tinted from the palette, centered on the style's
// auto-raise tool-button panel. Its current subclasses are the tab row's
// per-tab close/unpin button (TabCloseButton) and new-tab + button
// (NewTabButton). It carries everything mechanical such buttons have in common
// and nothing that differs between them.
//
// The single axis of divergence is which glyph to paint right now, expressed as
// two pure virtuals: glyphPath() and glyphRole(), its tint. paintEvent calls
// both every repaint, so a subclass whose glyph is backed by live state (a
// page's pin bit) needs no cached flag and no manual sync — pin/unpin anywhere
// shows up on the next repaint. A subclass with a fixed glyph just returns
// constants.
//
// A checkable button, while it's checked, fills its panel with
// GLYPH_BUTTON_CHECKED_PANEL_ROLE (the accent) and draws its glyph in
// GLYPH_BUTTON_CHECKED_ICON_ROLE in place of glyphRole(), so "on" reads at a
// glance, as a toggle switch that's on does. The hover and press wash still go
// over the fill. A button that isn't checkable is never checked, so it never
// has the fill.
//
// The render goes through one Glyph::Cache. Path and tint are both in its key,
// so a live swap (pin/unpin, a palette or color-group change) costs one render
// and every other repaint — hover, press, release — costs none.
//
// The panel is the style's flat tool-button panel (ButtonWash): nothing at
// rest, a wash on hover, a deeper one on press. Every subclass paints the
// identical panel from this identical code, so none can drift from another —
// there is no "real" QToolButton for any of them to chase.
//
// Deliberately behavior-free: it emits clicked() (from QAbstractButton) and
// nothing more. What a click means is wired per button by the owner (on the
// tab row, TabPaneLeaf: new-tab vs close/unpin), exactly as QTabBar keeps close
// handling in the bar rather than the button.
//
// Size is fixed at construction from the subclass's UiConstants knobs (extent =
// the button's square footprint; iconExtent = the centered glyph, the
// difference being padding), not from a style metric. The obvious one for the
// tab row, PM_TabCloseIndicator, runs tiny on some styles.
//
// Not for buttons that can't sit on the style's chrome. ZoomButton_ floats
// over document content, so it paints its own flat wash and a fixed light tint;
// it shares only the SVG render and its cache (Glyph.h) with this family
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
