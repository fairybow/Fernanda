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

#include <QFont>
#include <QFontMetrics>
#include <QList>
#include <QPainter>
#include <QPalette>
#include <QPen>
#include <QRect>
#include <QRectF>
#include <QString>
#include <QStringList>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QtMinMax>

#include "ui/UiConstants.h"

// Short labels at the right end of an item-view row, in muted capitals: a vault
// tree's file extensions ("PDF"), Go to File's "RECENT" and "COMMON". Free
// functions for the delegates that paint them, so the labels look the same
// wherever they appear.
//
// A delegate finds the labels' places (rects), shortens the row's name to end
// before the leftmost (nameElidedBefore), lets the style draw the row, then
// paints each label (paint). The letters are the palette's placeholder color,
// resolved for the row's state (colorGroupOf), and a label may have a thin
// rounded border of the same color, which tells two labels side by side apart
namespace Fernanda::Ui::RowBadges {

// The palette group an item paints in right now, the way QWidget::palette()
// picks one for a widget: Disabled if disabled, Normal (Active) if the view's
// window has focus, else Inactive
[[nodiscard]] inline QPalette::ColorGroup colorGroupOf(QStyle::State state)
{
    return !(state & QStyle::State_Enabled) ? QPalette::Disabled
           : (state & QStyle::State_Active) ? QPalette::Normal
                                            : QPalette::Inactive;
}

// The row's font scaled down. A font set in pixels has no point size, so it
// scales by pixels instead
[[nodiscard]] inline QFont font(const QFont& rowFont)
{
    auto font = rowFont;

    if (font.pointSizeF() > 0) {
        font.setPointSizeF(font.pointSizeF() * ROW_BADGE_FONT_SCALE);
    } else {
        font.setPixelSize(
            qMax(1, qRound(font.pixelSize() * ROW_BADGE_FONT_SCALE)));
    }

    return font;
}

// Where each label goes, in the order given, the last against the row's
// right edge and each other SPACING to the left of the next. Each is
// vertically centered and never taller than the row. The row's right edge is
// the view's own when, as in a vault tree or a list, the one column fills the
// viewport
[[nodiscard]] inline QList<QRect>
rects(const QRect& rowRect, const QStringList& badges, const QFont& badgeFont)
{
    QFontMetrics metrics(badgeFont);
    auto height =
        qMin(metrics.height() + 2 * ROW_BADGE_V_PADDING, rowRect.height());
    auto top = rowRect.top() + (rowRect.height() - height) / 2;
    auto right = rowRect.right() - ROW_BADGE_RIGHT_MARGIN;

    QList<QRect> result(badges.size());

    for (auto i = badges.size() - 1; i >= 0; --i) {
        auto width =
            metrics.horizontalAdvance(badges[i]) + 2 * ROW_BADGE_H_PADDING;

        result[i] = QRect(right - width + 1, top, width, height);
        right -= width + ROW_BADGE_SPACING;
    }

    return result;
}

// Where the style starts the row's name: its text rect's left edge plus a
// margin (QCommonStyle's PM_FocusFrameHMargin + 1)
[[nodiscard]] inline int
nameLeft(const QStyleOptionViewItem& option, const QStyle* style)
{
    auto text_rect = style->subElementRect(
        QStyle::SE_ItemViewItemText,
        &option,
        option.widget);
    auto text_margin = style->pixelMetric(
                           QStyle::PM_FocusFrameHMargin,
                           nullptr,
                           option.widget) +
                       1;

    return text_rect.left() + text_margin;
}

// The row's name, elided so it ends GAP short of x, the leftmost label's left
// edge, measured from where the style starts it (nameLeft). A name that
// already fits comes back unchanged, and the style's own elision then has
// nothing left to do
[[nodiscard]] inline QString
nameElidedBefore(const QStyleOptionViewItem& option, const QStyle* style, int x)
{
    auto room = x - ROW_BADGE_GAP - nameLeft(option, style);

    return QFontMetrics(option.font)
        .elidedText(option.text, option.textElideMode, qMax(room, 0));
}

// One label, in muted letters, with a border of the same color when
// bordered
inline void paint(
    QPainter* painter,
    const QStyleOptionViewItem& option,
    const QString& badge,
    const QFont& badgeFont,
    const QRect& badgeRect,
    bool bordered)
{
    auto color = option.palette.color(
        colorGroupOf(option.state),
        QPalette::PlaceholderText);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    if (bordered) {
        // Inset by half the pen so the whole stroke lands inside the rect
        auto half_pen = ROW_BADGE_BORDER_WIDTH / 2.0;
        painter->setPen(QPen(color, ROW_BADGE_BORDER_WIDTH));
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(
            QRectF(badgeRect)
                .adjusted(half_pen, half_pen, -half_pen, -half_pen),
            ROW_BADGE_RADIUS,
            ROW_BADGE_RADIUS);
    }

    painter->setFont(badgeFont);
    painter->setPen(color);
    painter->drawText(badgeRect, Qt::AlignCenter, badge);

    painter->restore();
}

} // namespace Fernanda::Ui::RowBadges
