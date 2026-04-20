/*
 * Hearth — a plain-text-first workbench for creative writing
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

#include <QFont>
#include <QFontMetricsF>
#include <QRectF>
#include <QString>

namespace Hearth::Painting {

// Centers horizontally only
inline QRectF
centeredGlyphRect(const QFont& font, const QString& glyph, const QRectF& bounds)
{
    QFontMetricsF metrics(font);
    auto glyph_rect = metrics.boundingRect(glyph);
    auto advance = metrics.horizontalAdvance(glyph);

    auto ink_center = glyph_rect.left() + glyph_rect.width() / 2.0;
    auto advance_center = advance / 2.0;
    auto correction = advance_center - ink_center;

    return bounds.adjusted(correction, 0, correction, 0);
}

} // namespace Hearth::Painting
