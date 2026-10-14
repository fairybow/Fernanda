/*
 * Suzuri — A plain-text editor for creative writing
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
#include <QFontMetrics>
#include <QJsonObject>
#include <QPaintEvent>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QRect>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QSize>
#include <QSplitter>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>
#include <QtMinMax>

#include <Coco/Debug.h>
#include <Coco/Path.h>
#include <Coco/Time.h>

#include "core/WorkspaceKeys.h"
#include "ui/Style.h"
#include "ui/UiConstants.h"
#include "ui/widgets/ButtonWash.h"
#include "ui/widgets/Glyph.h"

namespace Suzuri::Ui {

namespace Internal {

// The clickable title row of a Drawer. A checkable button that paints a
// disclosure chevron — Lucide ChevronDown while open, ChevronRight while
// collapsed — then, per COMMON_DRAWER_LABEL, the Common Vault glyph and/or the
// title, the title elided on the right if the header narrows (the glyphs never
// elide). Chevron, glyph, and title each take their own role
// (DRAWER_CHEVRON_ICON_ROLE, DRAWER_COMMON_VAULT_ICON_ROLE, DRAWER_TITLE_ROLE),
// seeded alike so the row reads as one color.
//
// Knows it's the Common Vault's header: it's the only Drawer, and nothing asks
// Drawer to be generic. If a second one ever appears, the label mode, glyph
// path, and role lift to constructor parameters.
//
// Its geometry is the DRAWER_* knobs in UiConstants.h. The height is fixed,
// and sizeHint reports it, which is what the Drawer's collapsed lock, restore
// check, and splitter borrow all read. Each glyph goes through its own
// Glyph::Cache. The chevron's path is in its key, so a toggle costs one render
// and every other repaint costs none. The Common Vault glyph's paint and
// measure are if constexpr on the mode, so in Title its Cache is never asked
// and renders nothing
class DrawerHeader_ : public QAbstractButton
{
    Q_OBJECT

public:
    explicit DrawerHeader_(QWidget* parentDrawer)
        : QAbstractButton(parentDrawer)
    {
        setup_();
    }

    ~DrawerHeader_() override { TRACER; }

    // The full label's width — every slot the mode shows, the title at full
    // advance — so the sidebar's minimum never cuts into it; eliding in
    // paintEvent only covers a header squeezed below its hint
    QSize sizeHint() const override
    {
        auto width = DRAWER_HEADER_LEFT_PADDING + DRAWER_CHEVRON_EXTENT +
                     DRAWER_CHEVRON_SPACING + RIGHT_PADDING_;

        if constexpr (COMMON_DRAWER_LABEL != DrawerLabel::Title) {
            width += DRAWER_COMMON_VAULT_ICON_EXTENT;
        }

        if constexpr (COMMON_DRAWER_LABEL == DrawerLabel::IconAndTitle) {
            width += DRAWER_COMMON_VAULT_ICON_SPACING;
        }

        if constexpr (COMMON_DRAWER_LABEL != DrawerLabel::Icon) {
            width += fontMetrics().horizontalAdvance(text());
        }

        return { width, height_() };
    }

    QSize minimumSizeHint() const override { return sizeHint(); }

protected:
    void paintEvent([[maybe_unused]] QPaintEvent* event) override
    {
        QPainter painter(this);
        paintWash_(painter);

        auto rect = contentsRect();
        auto x = rect.x() + DRAWER_HEADER_LEFT_PADDING;

        paintGlyph_(
            painter,
            chevron_,
            QString::fromLatin1(
                isChecked() ? CHEVRON_DOWN_ICON_PATH : CHEVRON_RIGHT_ICON_PATH),
            DRAWER_CHEVRON_EXTENT,
            DRAWER_CHEVRON_ICON_ROLE,
            x,
            rect);

        // Every slot advances by its unclamped extent, so a clamp never
        // shifts what follows
        x += DRAWER_CHEVRON_EXTENT + DRAWER_CHEVRON_SPACING;

        if constexpr (COMMON_DRAWER_LABEL != DrawerLabel::Title) {
            paintGlyph_(
                painter,
                commonVaultIcon_,
                QString::fromLatin1(COMMON_VAULT_ICON_PATH),
                DRAWER_COMMON_VAULT_ICON_EXTENT,
                DRAWER_COMMON_VAULT_ICON_ROLE,
                x,
                rect);

            x += DRAWER_COMMON_VAULT_ICON_EXTENT +
                 DRAWER_COMMON_VAULT_ICON_SPACING;
        }

        if constexpr (COMMON_DRAWER_LABEL != DrawerLabel::Icon) {
            paintTitle_(painter, x, rect);
        }
    }

private:
    // Room kept right of the label's last slot (the title, or the glyph in
    // Icon), in the width hint and when eliding
    static constexpr auto RIGHT_PADDING_ = 8;

    Glyph::Cache chevron_{};
    Glyph::Cache commonVaultIcon_{};

    // The row's height, with the padding a Row wash keeps above and below
    // itself. A Band fills the header, so it adds none
    [[nodiscard]] static constexpr int height_()
    {
        return DRAWER_HEADER_WASH == DrawerHeaderWash::Row
                   ? DRAWER_HEADER_HEIGHT + 2 * DRAWER_HEADER_ROW_PADDING
                   : DRAWER_HEADER_HEIGHT;
    }

    // Hover and press, per DRAWER_HEADER_WASH. Row is a flat button's wash
    // (ButtonWash), inset as a tree row's is, so the header reads as one more
    // row. Band fills the header square and edge to edge, in the same color,
    // short of its bottom pixel, where the collapsed drawer draws its line.
    // Being open isn't a pressed state either way: the chevron says that
    void paintWash_(QPainter& painter) const
    {
        if constexpr (DRAWER_HEADER_WASH == DrawerHeaderWash::Row) {
            ButtonWash::paint(
                painter,
                this,
                rect().adjusted(
                    STYLE_ROW_INSET,
                    DRAWER_HEADER_ROW_PADDING,
                    -STYLE_ROW_INSET,
                    -DRAWER_HEADER_ROW_PADDING));
        } else {
            if (isEnabled() && (underMouse() || isDown())) {
                painter.fillRect(
                    rect().adjusted(0, 0, 0, -1),
                    Style::buttonWash(palette(), isDown()));
            }
        }
    }

    void setup_()
    {
        setCheckable(true);
        setFocusPolicy(Qt::NoFocus);
        setAttribute(Qt::WA_Hover, true); // for the hover wash
        setFixedHeight(height_());
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

    // One glyph at x, vertically centered in rect. Clamped to the row, so an
    // over-large extent fills it rather than overflowing, and skipped at <= 0
    // (Glyph::Cache's contract). Resolved against the header's current color
    // group (QWidget::palette() picks it), like every glyph (UiConstants.h)
    void paintGlyph_(
        QPainter& painter,
        const Glyph::Cache& cache,
        const Coco::Path& svgPath,
        int extent,
        QPalette::ColorRole role,
        int x,
        const QRect& rect) const
    {
        auto clamped = qMin(extent, rect.height());
        if (clamped <= 0) {
            return;
        }

        auto pixmap = cache.pixmap(
            svgPath,
            clamped,
            palette().color(role),
            devicePixelRatioF());
        painter.drawPixmap(x, rect.y() + (rect.height() - clamped) / 2, pixmap);
    }

    // The title from x to the right padding, elided on the right
    void paintTitle_(QPainter& painter, int x, const QRect& rect) const
    {
        auto text_rect = QRect(
            x,
            rect.y(),
            rect.right() + 1 - RIGHT_PADDING_ - x,
            rect.height());
        if (text_rect.width() <= 0) {
            return;
        }

        painter.setPen(palette().color(DRAWER_TITLE_ROLE));
        painter.drawText(
            text_rect,
            Qt::AlignLeft | Qt::AlignVCenter,
            fontMetrics()
                .elidedText(text(), Qt::ElideRight, text_rect.width()));
    }
};

} // namespace Internal

// A collapsible section for a vertical QSplitter. Header toggles; content is
// any widget. Manages its own collapse (fixed height when closed) and, when it
// lives in a splitter, borrows/returns height from the sibling directly above
// it so expanding doesn't leave the content clipped at zero.
//
// Produces its own persisted state — { collapsed, height }, Obsidian's sidebar
// shape with height for width — as an opaque blob WorkspaceFile places but
// never reads into, the same split the trees' expansion follows. HEIGHT is the
// open height, kept while collapsed
class Drawer : public QWidget
{
    Q_OBJECT

public:
    Drawer(const QString& title, QWidget* content, QSplitter* parentSplitter)
        : QWidget(parentSplitter)
        , content_(content)
        , splitter_(parentSplitter)
    {
        setup_(title);
    }

    ~Drawer() override { TRACER; }

    bool isExpanded() const noexcept { return expanded_; }

    // Ground truth, no parallel bookkeeping: collapsed is !expanded_, and the
    // height is the open one (see openHeight_). Height is omitted until the
    // drawer has been opened once, so the first open keeps its fallback
    [[nodiscard]] QJsonObject serializeState() const
    {
        QJsonObject state{};
        state[WorkspaceKeys::DRAWER_COLLAPSED] = !expanded_;

        if (auto height = openHeight_(); height > 0) {
            state[WorkspaceKeys::DRAWER_HEIGHT] = height;
        }

        return state;
    }

    // Runs pre-show (WorkspaceFile::restore), when the splitter's sizes mean
    // nothing yet. So the flag, constraints, content, and remembered height
    // apply now — the arrow and content are right on the first frame — and only
    // the splitter sizing waits for the first show (see showEvent). Anything
    // unusable (absent object, missing or too-small height) leaves the drawer
    // as constructed: collapsed, with the first open using the fallback. Only
    // an explicit collapsed == false with a usable height restores it open
    void restoreState(const QJsonObject& state)
    {
        auto height = state.value(WorkspaceKeys::DRAWER_HEIGHT).toInt(-1);
        if (height <= header_->sizeHint().height()) {
            return;
        }

        lastExpandedSize_ = height;

        if (state.value(WorkspaceKeys::DRAWER_COLLAPSED).toBool(true)) {
            return;
        }

        applyExpansion_(true, false);
        pendingRestoreSizing_ = true;
    }

signals:
    // The persisted state changed: a user toggle, or a drag of this splitter's
    // handle (the only one — tree | drawer). Programmatic sizing (restore's
    // deferred pass, a toggle's borrow/return) emits nothing of its own; the
    // toggle's emission covers it
    void stateChanged();

protected:
    void paintEvent(QPaintEvent* event) override
    {
        QWidget::paintEvent(event);

        if (expanded_) {
            return;
        }

        // The collapsed drawer's bottom edge, drawn as the splitter handle
        // above it is: a divider line across the sidebar's whole width
        QPainter painter(this);
        painter
            .fillRect(0, height() - 1, width(), 1, Style::divider(palette()));
    }

    // Restore's deferred splitter sizing, one tick past the first show — the
    // same deferral the views use for scroll, for the same reason: sizes are
    // only valid post-layout. Re-checked on the tick, since a toggle in between
    // clears it
    void showEvent(QShowEvent* event) override
    {
        QWidget::showEvent(event);

        if (pendingRestoreSizing_) {
            Coco::Time::onNextTick(this, [this] {
                if (!pendingRestoreSizing_) {
                    return;
                }

                pendingRestoreSizing_ = false;
                requestSplitterSpace_(header_->sizeHint().height());
            });
        }
    }

private:
    QWidget* content_;
    QSplitter* splitter_;

    Internal::DrawerHeader_* header_ = new Internal::DrawerHeader_(this);
    bool expanded_ = false;
    int lastExpandedSize_ = -1;

    // Restored open, splitter sizing not yet applied (see restoreState /
    // showEvent). While set, height() is pre-layout noise, so openHeight_
    // reports lastExpandedSize_ instead
    bool pendingRestoreSizing_ = false;

    void setup_(const QString& title)
    {
        auto layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        // Set in every mode: Icon doesn't paint it, but it stays the header's
        // accessible name (QAbstractButton reads text()), and there it's the
        // tooltip, so a hover explains the lone glyph
        header_->setText(title);
        if constexpr (COMMON_DRAWER_LABEL == DrawerLabel::Icon) {
            header_->setToolTip(title);
        }

        content_->setParent(this);
        content_->setVisible(false);

        layout->addWidget(header_, 0);
        layout->addWidget(content_, 1);

        // Start collapsed, lock to header height
        setFixedHeight(header_->sizeHint().height());

        connect(
            header_,
            &QAbstractButton::toggled,
            this,
            &Drawer::onHeaderToggled_);

        if (splitter_) {
            connect(
                splitter_,
                &QSplitter::splitterMoved,
                this,
                &Drawer::stateChanged);
        }
    }

    // The height to reopen to / persist: the live height while open and laid
    // out, else the remembered one
    [[nodiscard]] int openHeight_() const
    {
        return (expanded_ && !pendingRestoreSizing_) ? height()
                                                     : lastExpandedSize_;
    }

    // Sizes this drawer to its open height in the parent QSplitter (if any),
    // trading with the sibling above. Without this, expanding leaves the
    // content clipped at zero height until the user drags the handle. Sets the
    // target exactly — grows or shrinks — since restore's deferred pass can
    // find the drawer laid out taller than its saved height (a toggle always
    // grows from header height). Clamped so the sibling above keeps its
    // minimum: a saved height taller than a now-shorter window would
    // otherwise drive it negative
    void requestSplitterSpace_(int headerHeight)
    {
        if (!splitter_) {
            return;
        }

        auto index = splitter_->indexOf(this);
        if (index < 1) {
            return;
        }

        auto above = static_cast<qsizetype>(index) - 1;
        auto sizes = splitter_->sizes();

        auto total = 0;
        for (auto s : sizes) {
            total += s;
        }

        // Restore last size, or fall back to ~1/3 of the splitter (capped
        // 250px)
        auto target = lastExpandedSize_ > headerHeight ? lastExpandedSize_
                                                       : qMin(total / 3, 250);

        auto above_floor = splitter_->widget(above)->minimumSizeHint().height();
        target = qMin(target, sizes[above] + sizes[index] - above_floor);
        if (target <= headerHeight) {
            return;
        }

        auto diff = target - sizes[index];
        if (diff == 0) {
            return;
        }

        sizes[above] -= diff;
        sizes[index] = target;
        splitter_->setSizes(sizes);
    }

    void releaseSplitterSpace_(int headerHeight)
    {
        if (!splitter_) {
            return;
        }

        auto index = splitter_->indexOf(this);
        if (index < 1) {
            return;
        }

        auto sizes = splitter_->sizes();

        auto freed = sizes[index] - headerHeight;
        if (freed <= 0) {
            return;
        }

        sizes[static_cast<qsizetype>(index) - 1] += freed;
        sizes[index] = headerHeight;
        splitter_->setSizes(sizes);
    }

    // The one expansion path. A toggle adjusts the splitter; restore doesn't
    // (pre-show, its sizes mean nothing — restoreState defers that part). The
    // header is set under a blocker so restore can't re-enter through toggled
    void applyExpansion_(bool expanded, bool adjustSplitter)
    {
        // Captured before expanded_ flips, so openHeight_ still reads the open
        // state (and a still-pending restored height, not pre-layout noise)
        if (!expanded) {
            lastExpandedSize_ = openHeight_();
        }

        pendingRestoreSizing_ = false;
        expanded_ = expanded;
        content_->setVisible(expanded);

        {
            QSignalBlocker blocker(header_);
            header_->setChecked(expanded);
        }

        auto header_h = header_->sizeHint().height();

        if (expanded) {
            // Undo the fixed height; minimum is just the header so it can
            // shrink almost all the way back down
            setMinimumHeight(header_h);
            setMaximumHeight(QWIDGETSIZE_MAX);
            if (adjustSplitter) {
                requestSplitterSpace_(header_h);
            }
        } else {
            setFixedHeight(header_h);
            if (adjustSplitter) {
                releaseSplitterSpace_(header_h);
            }
        }
    }

    void onHeaderToggled_(bool checked)
    {
        applyExpansion_(checked, true);
        emit stateChanged();
    }
};

} // namespace Suzuri::Ui
