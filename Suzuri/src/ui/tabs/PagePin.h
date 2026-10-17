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

#include <QVariant>
#include <QWidget>

namespace Suzuri::Ui {

// Pinned state for a tab page. It lives ON the page widget, not on the hosting
// tab, because the page pointer is the identity that already rides through
// every move: it is the leaf's tabData payload (so it survives an in-leaf
// reorder), it is the drag payload (so it survives a move to another leaf or a
// pop-out), and it is the same object a replace or a restore hands back.
// Storing the bit on that object means all of those carry it for free — no
// DragState_ field, no addPage / takePage parameter.
//
// A dynamic property, rather than a typed member, keeps this decoupled from the
// two unrelated page types: AbstractFileView and NewTabPage share only QWidget.
// A TabPaneLeaf reads isPagePinned() to render the pin indicator exactly as it
// reads windowTitle for the tab text, so it stays page-type-ignorant. The one
// stringly-typed key is centralized here.
//
// A possible consolidation (docs/TODO.md): a shared TabPage : QWidget base
// carrying pin state and title as typed members, uniting what currently hangs
// off QWidget in two places. It trades a qobject_cast in the leaf for that
// consolidation

inline constexpr auto PAGE_PINNED_PROPERTY = "suzuri_pagePinned";

[[nodiscard]] inline bool isPagePinned(QWidget* page)
{
    return page && page->property(PAGE_PINNED_PROPERTY).toBool();
}

// Set by the pin/unpin context action; read here and by the leaf's indicator
// and the workspace serializer
inline void setPagePinned(QWidget* page, bool pinned)
{
    if (page) {
        page->setProperty(PAGE_PINNED_PROPERTY, pinned);
    }
}

} // namespace Suzuri::Ui
