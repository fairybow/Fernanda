/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
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

#include <QAbstractButton>
#include <QPainter>
#include <QRect>
#include <QStyle>
#include <QStyleOption>

namespace Suzuri::Ui::ButtonWash {

// The hover and press wash of a flat button, drawn by the style into rect:
// nothing at rest, a wash under the pointer, a deeper one while held down. The
// option is the one a flat QToolButton builds (State_AutoRaise, the hover state
// initFrom reads, State_Sunken while down), so the style draws exactly what it
// draws for one. Shared by the glyph buttons (GlyphButton) and the Common Vault
// drawer's header, which aren't QToolButtons but should look like them
inline void
paint(QPainter& painter, const QAbstractButton* button, const QRect& rect)
{
    QStyleOptionToolButton option{};
    option.initFrom(button);
    option.rect = rect;
    option.state |= QStyle::State_AutoRaise;

    if (button->isDown()) {
        option.state |= QStyle::State_Sunken;
    }

    button->style()
        ->drawPrimitive(QStyle::PE_PanelButtonTool, &option, &painter, button);
}

} // namespace Suzuri::Ui::ButtonWash
