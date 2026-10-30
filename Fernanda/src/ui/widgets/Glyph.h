/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
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

#include <QColor>
#include <QIcon>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QRectF>
#include <QSvgRenderer>
#include <QtMinMax>

#include <Coco/Path.h>

namespace Fernanda::Ui::Glyph {

namespace Internal {

// How many times larger than its canvas a glyph is drawn before it's scaled
// down to it (render)
inline constexpr int SUPERSAMPLE_ = 4;

} // namespace Internal

// Render a Lucide SVG into a tinted pixmap ourselves (Qt SVG), so the caller
// owns its size and color rather than handing it to a QIcon theme. SourceIn
// recolors it to the passed color, keeping it legible on any background.
//
// The glyph consumers agree exactly on the render and nowhere else, which is
// why this is a free function and not a shared button base. Every Lucide glyph
// in the app comes through here, most via Cache below.
//
// Pure over its arguments: the SVGs are compiled-in resources, so path, extent,
// color, and DPR fully determine the result. Cache depends on that.
//
// Styling is deliberately minimal. If this ever routes through an SVG-in-QSS
// pipeline, how and where this renders can change without touching any consumer
// that calls it.
//
// The glyph is drawn SUPERSAMPLE_ times larger and scaled down, rather than
// drawn at its size. Lucide's stroke is a twelfth of the glyph, so a 12-pixel
// glyph on a 100% display has a 1-pixel stroke, and Qt draws lines that thin
// with a separate, faster stroker. That one places lines half a pixel off and
// spreads each across two pixels evenly, so diagonals come out as uneven
// staircases (a lopsided X or chevron) and round caps are lost. Drawn larger,
// every stroke is several pixels wide and goes through the ordinary stroker;
// scaling down then averages each block of pixels into one, which is the
// coverage the glyph would have at its own size
[[nodiscard]] inline QPixmap
render(const Coco::Path& svgPath, int extent, const QColor& color, qreal dpr)
{
    dpr = qMax(qreal(1), dpr);
    QSvgRenderer renderer(svgPath.toQString());

    auto pixels = qRound(extent * dpr);
    auto large = pixels * Internal::SUPERSAMPLE_;

    QImage image(large, large, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    renderer.render(&painter, QRectF(0, 0, large, large));
    painter.end();

    image = image.scaled(
        pixels,
        pixels,
        Qt::IgnoreAspectRatio,
        Qt::SmoothTransformation);

    painter.begin(&image);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(image.rect(), color);
    painter.end();

    auto pixmap = QPixmap::fromImage(image);
    pixmap.setDevicePixelRatio(dpr);
    return pixmap;
}

// A one-slot memo of render: holds the last glyph rendered and the arguments it
// was rendered from, and re-renders only when a request's arguments differ.
// Because render is pure, keying on all four of its arguments means a Cache can
// never hand back a stale glyph — there is no input it doesn't see. A null
// pixmap means never rendered.
//
// One Cache per glyph a consumer shows at the same time. A consumer that paints
// two glyphs in one pass (a tree's collapsed and expanded rows) holds two, or
// the single slot would re-render on every alternation. A consumer whose one
// glyph swaps on live state (a pin bit) can hold one: the path is in the key,
// so a swap costs one render and every ordinary repaint costs none.
//
// Const-callable: it's filled from const paint paths (drawBranches,
// initStyleOption), so its state is mutable here rather than at every
// consumer, which holds a plain member.
//
// Both accessors return by value. A reference into a one-slot cache would
// dangle on the next request that re-renders; QPixmap and QIcon are implicitly
// shared, so the copy is a reference-count bump.
//
// Callers keep their own extent clamping and skip the call when the clamped
// extent is <= 0: render makes a null pixmap there, which this would read as
// never rendered and redo on every request
class Cache
{
public:
    [[nodiscard]] QPixmap pixmap(
        const Coco::Path& svgPath,
        int extent,
        const QColor& color,
        qreal dpr) const
    {
        refresh_(svgPath, extent, color, dpr);
        return pixmap_;
    }

    // For consumers that hand the glyph to the style as a QIcon (the tree
    // delegate's row decorations, a tab's mark). Built lazily from the cached
    // pixmap and kept alongside it, so any mode the style asks the icon for
    // (Disabled, say), which the icon's engine caches per instance, survives
    // across paints. Discarded whenever the pixmap re-renders
    [[nodiscard]] QIcon
    icon(const Coco::Path& svgPath, int extent, const QColor& color, qreal dpr)
        const
    {
        refresh_(svgPath, extent, color, dpr);
        if (icon_.isNull()) {
            icon_ = QIcon(pixmap_);
        }
        return icon_;
    }

private:
    mutable QPixmap pixmap_{}; // null = never rendered
    mutable QIcon icon_{};
    mutable Coco::Path renderedPath_{};
    mutable int renderedExtent_ = 0;
    mutable QColor renderedColor_{};
    mutable qreal renderedDpr_ = 0;

    void refresh_(
        const Coco::Path& svgPath,
        int extent,
        const QColor& color,
        qreal dpr) const
    {
        if (!pixmap_.isNull() && svgPath == renderedPath_ &&
            extent == renderedExtent_ && color == renderedColor_ &&
            dpr == renderedDpr_) {
            return;
        }

        pixmap_ = render(svgPath, extent, color, dpr);
        icon_ = QIcon();
        renderedPath_ = svgPath;
        renderedExtent_ = extent;
        renderedColor_ = color;
        renderedDpr_ = dpr;
    }
};

} // namespace Fernanda::Ui::Glyph
