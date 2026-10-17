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

#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QString>
#include <QWidget>
#include <QtMinMax>

#include "ui/UiConstants.h"

namespace Suzuri::Ui {

template <typename SlotT>
inline QPushButton*
buildPushButton(const QString& text, QWidget* parent, SlotT&& slot)
{
    auto button = new QPushButton(text, parent);
    parent->connect(button, &QPushButton::clicked, parent, slot);

    return button;
}

// Give a widget's digits one shared width (the font's "tnum" feature), so a
// number that changes in place — a count, a position — doesn't nudge the text
// beside it as its digits change. A font without the feature is unaffected.
// QFont::setFeature is Qt 6.7
inline void setTabularNumerals(QWidget* widget)
{
    auto font = widget->font();
    font.setFeature(QFont::Tag("tnum"), 1);
    widget->setFont(font);
}

// The rounded label a drag carries under the cursor. One implementation for
// both drag sources — TabPaneTree's tabs and VaultTreeView's entries — so the
// two drags can't drift apart. font is the source widget's, so the label is
// drawn in the same face as the thing it came from
[[nodiscard]] inline QPixmap dragPixmap(const QString& text, const QFont& font)
{
    QFontMetrics fm(font);
    auto width = qMin(MAX_DRAG_PIXMAP_WIDTH, fm.horizontalAdvance(text) + 24);
    auto height = fm.height() + 12;

    QPixmap pixmap(width, height);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(45, 45, 45, 225));
    painter.drawRoundedRect(pixmap.rect().adjusted(0, 0, -1, -1), 4, 4);

    painter.setPen(Qt::white);
    painter.drawText(
        pixmap.rect(),
        Qt::AlignCenter,
        fm.elidedText(text, Qt::ElideRight, width - 16));

    return pixmap;
}

} // namespace Suzuri::Ui
