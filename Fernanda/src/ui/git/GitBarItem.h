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
#include <QFont>
#include <QFontMetrics>
#include <QPaintEvent>
#include <QPainter>
#include <QPalette>
#include <QRect>
#include <QSize>
#include <QString>
#include <QWidget>
#include <QtMinMax>

#include <Coco/Debug.h>
#include <Coco/Path.h>

#include "ui/UiConstants.h"
#include "ui/widgets/ButtonWash.h"
#include "ui/widgets/Glyph.h"

namespace Fernanda::Ui {

using namespace Qt::StringLiterals;

// One section of the Git view's top bar, as GitHub Desktop draws its own: a
// glyph, then a small muted caption ("Current branch") over a name ("main"),
// and, for one that opens a menu, a chevron at its right edge. The name
// elides when the section is narrower than it; the caption and glyphs never
// move.
//
// A section that does something is a flat button, with the style's wash on
// hover and press (ButtonWash). One that only shows something (the
// repository) is made inert: it takes no mouse events, so it has no wash
// either. What a click means is its owner's (GitTopBar)
class GitBarItem : public QAbstractButton
{
    Q_OBJECT

public:
    // glyphPath and caption are fixed; the name is set later (setName).
    // opensMenu adds the chevron and makes the section a button
    GitBarItem(
        const QString& glyphPath,
        const QString& caption,
        bool opensMenu,
        QWidget* parentTopBar)
        : QAbstractButton(parentTopBar)
        , glyphPath_(glyphPath)
        , caption_(caption)
        , opensMenu_(opensMenu)
    {
        setup_();
    }

    ~GitBarItem() override { TRACER; }

    void setName(const QString& name)
    {
        if (name == text()) {
            return;
        }

        setText(name);
        setToolTip(name);
        update();
    }

    QSize sizeHint() const override
    {
        return { GIT_BAR_ITEM_WIDTH, GIT_BAR_HEIGHT };
    }

protected:
    void paintEvent([[maybe_unused]] QPaintEvent* event) override
    {
        QPainter painter(this);

        if (opensMenu_) {
            ButtonWash::paint(painter, this, rect());
        }

        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

        auto x = GIT_BAR_ITEM_PADDING;
        paintGlyph_(painter, glyph_, glyphPath_, GIT_BAR_ICON_EXTENT, x);
        x += GIT_BAR_ICON_EXTENT + GIT_BAR_ICON_SPACING;

        auto right = width() - GIT_BAR_ITEM_PADDING;

        if (opensMenu_) {
            auto chevron_x = right - GIT_BAR_CHEVRON_EXTENT;
            paintGlyph_(
                painter,
                chevron_,
                QString::fromLatin1(CHEVRON_DOWN_ICON_PATH),
                GIT_BAR_CHEVRON_EXTENT,
                chevron_x);
            right = chevron_x - GIT_BAR_ICON_SPACING;
        }

        paintText_(painter, x, qMax(0, right - x));
    }

private:
    QString glyphPath_;
    QString caption_;
    bool opensMenu_;

    Glyph::Cache glyph_{};
    Glyph::Cache chevron_{};

    void setup_()
    {
        setFocusPolicy(Qt::NoFocus);
        setFixedHeight(GIT_BAR_HEIGHT);
        setMinimumWidth(GIT_BAR_ITEM_MIN_WIDTH);

        if (opensMenu_) {
            setAttribute(Qt::WA_Hover, true);
        } else {
            setAttribute(Qt::WA_TransparentForMouseEvents, true);
        }
    }

    // One glyph at x, vertically centered, in the section's icon tint
    void paintGlyph_(
        QPainter& painter,
        const Glyph::Cache& cache,
        const QString& path,
        int extent,
        int x) const
    {
        auto pixmap = cache.pixmap(
            path,
            extent,
            palette().color(GIT_BAR_ICON_ROLE),
            devicePixelRatioF());

        painter.drawPixmap(x, (height() - extent) / 2, pixmap);
    }

    // The caption over the name, the two centered as a block in the height
    void paintText_(QPainter& painter, int x, int room) const
    {
        auto caption_font = font();

        if (caption_font.pointSizeF() > 0) {
            caption_font.setPointSizeF(
                caption_font.pointSizeF() * GIT_BAR_CAPTION_SCALE);
        } else {
            caption_font.setPixelSize(qMax(
                1,
                qRound(caption_font.pixelSize() * GIT_BAR_CAPTION_SCALE)));
        }

        auto name_font = font();
        name_font.setBold(true);

        QFontMetrics caption_metrics(caption_font);
        QFontMetrics name_metrics(name_font);

        auto block = caption_metrics.height() + name_metrics.height();
        auto top = (height() - block) / 2;

        painter.setFont(caption_font);
        painter.setPen(palette().color(GIT_BAR_CAPTION_ROLE));
        painter.drawText(
            QRect(x, top, room, caption_metrics.height()),
            Qt::AlignLeft | Qt::AlignVCenter,
            caption_metrics.elidedText(caption_, Qt::ElideRight, room));

        painter.setFont(name_font);
        painter.setPen(palette().color(QPalette::ButtonText));
        painter.drawText(
            QRect(
                x,
                top + caption_metrics.height(),
                room,
                name_metrics.height()),
            Qt::AlignLeft | Qt::AlignVCenter,
            name_metrics.elidedText(text(), Qt::ElideRight, room));
    }
};

} // namespace Fernanda::Ui
