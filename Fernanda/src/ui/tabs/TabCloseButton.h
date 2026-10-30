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

#include <QString>
#include <QWidget>

#include <Coco/Debug.h>

#include "ui/UiConstants.h"
#include "ui/tabs/PagePin.h"
#include "ui/widgets/GlyphButton.h"

namespace Fernanda::Ui {

using namespace Qt::StringLiterals;

// The per-tab close/unpin button, installed on a tab's close side via
// QTabBar::setTabButton (TabPaneLeaf does the installing). It carries the one
// identity everything in the leaf already rides on — the page pointer — not an
// index and not a stored pinned flag.
//
// It is deliberately dumb. It renders one glyph and emits clicked(); the leaf
// decides what a click means (unpin vs close), exactly as QTabBar keeps close
// handling in the bar rather than in the button. There is no bool pinned_ here:
// the pin bit lives on the page (PagePin) and is the single source of truth, so
// the glyph is read live off the page every paint (glyphPath) — pin or unpin
// anywhere shows up on the next repaint with nothing to keep in sync.
//
// No hover glyph swap: current Obsidian doesn't reveal the close X under the
// pin on hover, and neither do we — the glyph never changes on hover. The frame
// (auto-raise tool-button panel) and all other mechanics live in GlyphButton,
// shared with NewTabButton.
//
// Lifetime is Qt's: a QTabBar owns its tab buttons and deleteLater()s them when
// the tab is removed, so this button (and the leaf's click lambda bound to it)
// dies exactly when its tab does — no manual cleanup, and the lambda can never
// fire on a dangling page
class TabCloseButton : public GlyphButton
{
    Q_OBJECT

public:
    explicit TabCloseButton(QWidget* page, QWidget* parent)
        : GlyphButton(TAB_BUTTON_EXTENT, TAB_BUTTON_ICON_EXTENT, parent)
        , page_(page)
    {
    }

    ~TabCloseButton() override { TRACER; }

    [[nodiscard]] QWidget* page() const { return page_; }

    // replacePage swaps the page under a persistent tab, so the button (which
    // persists with the tab) is repointed at the new page. update() refreshes
    // the glyph to the new page's pin state
    void setPage(QWidget* page)
    {
        page_ = page;
        update();
    }

protected:
    // Glyph read live off the page's pin bit (PagePin) — the page is the single
    // source of truth, so this button holds no glyph state of its own
    [[nodiscard]] Coco::Path glyphPath() const override
    {
        return isPagePinned(page_) ? u":/lucide/Pin.svg"_s
                                   : u":/lucide/X.svg"_s;
    }

    [[nodiscard]] QPalette::ColorRole glyphRole() const override
    {
        return isPagePinned(page_) ? TAB_PIN_ICON_ROLE : TAB_CLOSE_ICON_ROLE;
    }

private:
    QWidget* page_;
};

} // namespace Fernanda::Ui
