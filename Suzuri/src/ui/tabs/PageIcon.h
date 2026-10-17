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

#include <QString>
#include <QVariant>
#include <QWidget>

namespace Suzuri::Ui {

// A tab mark for a page: the small glyph its tab draws ahead of the title. It
// lives ON the page widget for the same reasons the pin does (PagePin): the
// page pointer is the identity that already rides through every move — it is
// the leaf's tabData payload, the drag payload, and the object a replace or a
// restore hands back — so the mark survives a reorder, a move to another leaf
// or a pop-out, and a workspace restore, with no addPage / takePage parameter
// and no DragState_ field.
//
// A dynamic property, rather than a typed member, keeps this decoupled from the
// two unrelated page types: AbstractFileView and NewTabPage share only QWidget.
// A TabPaneLeaf reads pageIconPath to draw the tab exactly as it reads
// windowTitle for the tab text and isPagePinned for the button glyph, so it
// stays page-type-ignorant. The one stringly-typed key is centralized here.
//
// The page carries which glyph (an SVG resource path), never a rendered icon.
// The leaf renders it in its own colors and renders it again when those change
// (TabPaneLeaf::changeEvent), so a mark follows a light or dark switch like
// every other glyph. An icon rendered once and carried here would keep the
// colors of the moment it was made.
//
// What a mark MEANS is the window's business, not the leaf's. VaultWindow sets
// one on a view whose file belongs to a vault other than the window's own — a
// Common Vault file open in a project window — and never in the Common Vault's
// own window, where the two vaults are one object. The leaf just draws whatever
// it finds, so a later use (per-file-type glyphs, say) needs nothing here.
//
// Set once, at view construction, and never updated: a file can't change vaults
// under a view (Vault::relocate_ keeps both ends inside one vault), and a tab
// dragged into another window's family is refused by familyId. So unlike the
// title, there is no change signal for the leaf to bind — the leaf applies the
// mark wherever a tab gains a page.
//
// The consolidation noted in PagePin would cover this too: a shared TabPage :
// QWidget base carrying pin, title, and mark as typed members would unite what
// currently hangs off QWidget in three places

inline constexpr auto PAGE_ICON_PATH_PROPERTY = "suzuri_pageIconPath";

// Empty when the page has no mark
[[nodiscard]] inline QString pageIconPath(QWidget* page)
{
    return page ? page->property(PAGE_ICON_PATH_PROPERTY).toString()
                : QString{};
}

// Set by VaultWindow when it builds a view (makeView_); read by the leaf when a
// tab gains a page (addPage, replacePage). An empty path clears the tab's mark
inline void setPageIconPath(QWidget* page, const QString& svgPath)
{
    if (page) {
        page->setProperty(PAGE_ICON_PATH_PROPERTY, svgPath);
    }
}

} // namespace Suzuri::Ui
