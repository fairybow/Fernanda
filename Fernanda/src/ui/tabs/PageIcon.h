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
#include <QVariant>
#include <QWidget>

namespace Fernanda::Ui {

// A page's tab mark: the small glyph its tab draws before the title. Like the
// pin (PagePin), it lives on the page widget, the identity that already rides
// through every move (tabData, the drag payload, what a replace or restore
// hands back), so it survives a reorder, a move to another leaf or a pop-out,
// and a restore, with no extra parameter. A dynamic property, not a typed
// member, since AbstractFileView and NewTabPage share only QWidget; the key
// lives here. TabPaneLeaf reads it as it reads windowTitle and isPagePinned,
// knowing nothing of page types.
//
// The page carries which glyph (an SVG resource path), never a rendered icon:
// the leaf renders it in its own colors, again when they change
// (TabPaneLeaf::changeEvent), so it follows light and dark.
//
// What a mark means is the window's business. VaultWindow marks a view whose
// file is another vault's (a Common Vault file in a project window), and never
// in the Common Vault's own window. It sets the mark at construction, and again
// when a file moved or renamed from another vault lands in the window's other
// vault, or the Common Vault changes. The leaf applies it wherever a tab gains
// a page; a change under a tab is drawn when the window asks
// (TabPaneTree::refreshPageIcon), as there's no change signal to bind. A tab
// can't be dragged into another window's family (familyId), so that never
// changes a mark.
//
// The shared TabPage base PagePin notes, carrying pin, title, and mark as
// typed members, would cover this too

inline constexpr auto PAGE_ICON_PATH_PROPERTY = "fernanda_pageIconPath";

// Empty when the page has no mark
[[nodiscard]] inline QString pageIconPath(QWidget* page)
{
    return page ? page->property(PAGE_ICON_PATH_PROPERTY).toString()
                : QString{};
}

// Set by VaultWindow; read by the leaf when a tab gains a page (addPage,
// replacePage) or when asked (refreshPageIcon). An empty path clears the tab's
// mark
inline void setPageIconPath(QWidget* page, const QString& svgPath)
{
    if (page) {
        page->setProperty(PAGE_ICON_PATH_PROPERTY, svgPath);
    }
}

} // namespace Fernanda::Ui
