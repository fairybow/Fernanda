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

#include <QIcon>
#include <QPaintEvent>
#include <QPalette>
#include <QRect>
#include <QString>
#include <QStyle>
#include <QStyleOptionToolButton>
#include <QStylePainter>
#include <QToolButton>
#include <QWidget>
#include <QtMinMax>

#include <Coco/Debug.h>
#include <Coco/Path.h>

#include "ui/UiConstants.h"
#include "ui/widgets/Glyph.h"

namespace Fernanda::Ui {

// A flat button that opens a menu, drawn as Obsidian draws its vault profile:
// a glyph, then the button's text left-aligned after it, in place of the
// style's label and menu indicator. The vault switcher and the Git page's
// branch button are this. What the menu holds is its owner's (setMenu, with
// InstantPopup).
//
// The style draws only the frame. paintEvent builds the option exactly as
// QToolButton's own does, clears HasMenu (the style draws its bottom-right
// indicator whenever the option carries it, and nothing short of QSS turns it
// off), and empties the label, so CC_ToolButton paints the panel's hover,
// press, and sunken-while-open states and nothing else. The glyph and text
// are painted here, from the MENU_BUTTON_* knobs in UiConstants.h. The
// style's label couldn't be kept: Qt 6.11's Windows 11 style centers the text
// beside an icon where 6.10 left-aligned it. Owning the layout also elides a
// long text on the right, and shows a '&' as itself rather than as a
// mnemonic.
//
// Everything else stays QToolButton's: setMenu / InstantPopup still position
// the popup (flipping it upward at the screen's bottom edge) and keep the
// button sunken while the menu is open. Only the painted option changes; the
// widget's own state never does. The glyph goes through a Glyph::Cache keyed on
// its tint, so it follows the current color group with no refresh bookkeeping
// and re-renders only when the resolved color (or the DPR) actually moves
class MenuButton : public QToolButton
{
    Q_OBJECT

public:
    MenuButton(const Coco::Path& glyphPath, QWidget* parent)
        : QToolButton(parent)
        , glyphPath_(glyphPath)
    {
    }

    ~MenuButton() override { TRACER; }

protected:
    void paintEvent([[maybe_unused]] QPaintEvent* event) override
    {
        QStylePainter painter(this);

        // Frame only: no indicator, no label
        QStyleOptionToolButton option;
        initStyleOption(&option);
        option.features &= ~QStyleOptionToolButton::HasMenu;
        option.text.clear();
        option.icon = QIcon();
        option.toolButtonStyle = Qt::ToolButtonTextOnly;
        painter.drawComplexControl(QStyle::CC_ToolButton, option);

        // Sink the content with the panel on press. The metrics are 0 on
        // styles that don't shift, so this is a no-op there
        auto content = rect();
        if (option.state & QStyle::State_Sunken) {
            content.translate(
                style()->pixelMetric(
                    QStyle::PM_ButtonShiftHorizontal,
                    &option,
                    this),
                style()->pixelMetric(
                    QStyle::PM_ButtonShiftVertical,
                    &option,
                    this));
        }

        auto glyph_x = content.x() + MENU_BUTTON_LEFT_PADDING;

        // Clamped to the button, so an over-large extent fills it rather than
        // overflowing
        auto extent = qMin(MENU_BUTTON_ICON_EXTENT, content.height());
        if (extent > 0) {
            auto glyph = glyph_.pixmap(
                glyphPath_,
                extent,
                palette().color(MENU_BUTTON_ICON_ROLE),
                devicePixelRatioF());
            painter.drawPixmap(
                glyph_x,
                content.y() + (content.height() - extent) / 2,
                glyph);
        }

        // Measured from the unclamped slot, so the text never shifts with a
        // clamp
        auto text_x =
            glyph_x + MENU_BUTTON_ICON_EXTENT + MENU_BUTTON_ICON_SPACING;
        auto text_rect = QRect(
            text_x,
            content.y(),
            content.right() + 1 - RIGHT_PADDING_ - text_x,
            content.height());
        if (text_rect.width() <= 0) {
            return;
        }

        painter.setPen(palette().color(QPalette::ButtonText));
        painter.drawText(
            text_rect,
            Qt::AlignLeft | Qt::AlignVCenter,
            fontMetrics()
                .elidedText(text(), Qt::ElideRight, text_rect.width()));
    }

private:
    // Room kept right of the text when eliding
    static constexpr auto RIGHT_PADDING_ = 8;

    Coco::Path glyphPath_;
    Glyph::Cache glyph_{};
};

} // namespace Fernanda::Ui
