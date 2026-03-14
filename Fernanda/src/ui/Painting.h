/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QFont>
#include <QFontMetricsF>
#include <QRectF>
#include <QString>

namespace Fernanda::Painting {

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

} // namespace Fernanda::Painting
