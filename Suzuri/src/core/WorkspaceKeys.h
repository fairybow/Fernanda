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

#include <QString>

namespace Suzuri::WorkspaceKeys {

using namespace Qt::StringLiterals;

// Every JSON key and value token in workspace.json — one place to read the
// whole schema, and the single source its write/read sites share so a mistyped
// literal can't silently break a restore (the reason action ids live in
// ActionIds.h). Grouped by where each sits in the file. Producers:
//   WorkspaceFile — the outer file structure (sections 1–2)
//   Drawer        — the common drawer's blob under COMMON_DRAWER (section 2)
//   TabPaneTree   — the split/leaf tree blob under TREE (section 3)
//   VaultWindow   — a tab entry inside a leaf's TABS (section 4), and the
//                   RECENT_FILES array (section 2)
//   the file views — the per-view STATE blob inside a tab entry (section 5)
//
// TYPE is deliberately shared: it is the object-type discriminator at every
// level, its value set differing by level (TYPE_SPLIT/TYPE_LEAF for a tree
// node, TYPE_NEWTAB for a tab). Everything else is level-local and cannot
// collide, and per-view keys sit inside their own STATE object by construction
// (Obsidian's opaque-state shape).

// --- 1. File structure (WorkspaceFile, top level) ---------------------------

inline const QString VERSION = u"version"_s; // schema version int
inline const QString MAIN = u"main"_s;       // the main window's object
inline const QString POPOUTS = u"popouts"_s; // array of pop-out objects

// --- 2. Window object (MAIN, and each POPOUTS entry) ------------------------

// Base64 QWidget::saveGeometry; on the main object and each pop-out
inline const QString GEOMETRY = u"geometry"_s;
// Base64 QSplitter::saveState for the window's sidebar | editor split — the
// sidebar's width. Not the Sidebar's inner tree | drawer split; the drawer's
// height rides in COMMON_DRAWER. Main only (pop-outs have none)
inline const QString SIDEBAR_SIZES = u"sidebarSizes"_s;
// The pane/tab tree blob (section 3); on main and each pop-out
inline const QString TREE = u"tree"_s;
// The two sidebar trees' folder expansion; main only
inline const QString EXPANDED = u"expanded"_s;

// Keys of the EXPANDED object: this vault's file tree, and the common drawer
// tree (absent when this window IS the common vault). Same spelling as
// VAULT_THIS / VAULT_COMMON below but a different role — kept separate so the
// two can never be conflated or drift together
inline const QString EXPANDED_THIS = u"this"_s;
inline const QString EXPANDED_COMMON = u"common"_s;

// The common drawer's state; main only, absent when this window IS the common
// vault. A Drawer-produced blob, like the trees' expansion arrays
inline const QString COMMON_DRAWER = u"commonDrawer"_s;

// Keys of the COMMON_DRAWER object, written and read by Drawer. Obsidian's
// sidebar shape ({ collapsed, width }), with height for width: DRAWER_HEIGHT is
// the open height in px, kept while collapsed. COLLAPSED rather than an
// "expanded" flag — Obsidian's name, and clear of the folder-EXPANDED key above
inline const QString DRAWER_COLLAPSED = u"collapsed"_s;
inline const QString DRAWER_HEIGHT = u"height"_s;

// The files most recently made active in this window or its pop-outs, newest
// first (RecentFiles); main only. An array of objects each with VAULT and
// FILE_PATH, as a tab entry has (section 4). Written by VaultWindow
inline const QString RECENT_FILES = u"recentFiles"_s;

// --- 3. Tree node (TabPaneTree, nested under TREE) --------------------------

// The node discriminator (see the TYPE note above)
inline const QString TYPE = u"type"_s;
inline const QString TYPE_SPLIT = u"split"_s; // a QSplitter node
inline const QString TYPE_LEAF = u"leaf"_s;   // a TabPaneLeaf node

// Split node: orientation, per-child sizes, children
inline const QString ORIENTATION = u"orientation"_s;
inline const QString ORIENTATION_V = u"v"_s; // Qt::Vertical
inline const QString ORIENTATION_H = u"h"_s; // Qt::Horizontal
inline const QString SIZES = u"sizes"_s;     // int array, one per child
inline const QString CHILDREN = u"children"_s;

// Leaf node: the active tab index (into the filtered list) + the tab array
inline const QString ACTIVE = u"active"_s;
inline const QString TABS = u"tabs"_s;

// --- 4. Tab entry (VaultWindow, an element of a leaf's TABS) ----------------

// A tab's TYPE is present only on a non-file tab (an empty NewTabPage); a file
// tab has none and is recognized by FILE_PATH
inline const QString TYPE_NEWTAB = u"newtab"_s; // TYPE value (shares the key)

// Which of the window's two vaults the file belongs to
inline const QString VAULT = u"vault"_s;
inline const QString VAULT_COMMON = u"common"_s; // VAULT value
inline const QString VAULT_THIS = u"this"_s;     // VAULT value

// The vault-relative path — the whole file identity. Named FILE_PATH, not FILE,
// to stay clear of the stdio identifier
inline const QString FILE_PATH = u"file"_s;

// Pin state. On both file and newtab entries
inline const QString PINNED = u"pinned"_s;

// The opaque per-view state blob — describePage_ nests each view's
// writeViewState output here; omitted when a view writes nothing
inline const QString STATE = u"state"_s;

// --- 5. Per-view state (the file views, inside STATE) -----------------------

// Scroll is a vertical + horizontal scrollbar pair, written by every view that
// scrolls — TextFileView, PdfFileView, and ImageFileView all write both axes.
// TextFileView's horizontal value is 0 while the editor wraps lines.
// ImageFileView writes 0,0 in Fit, where its bars are off. Each view reads back
// only the keys it knows. VIEW_CURSOR is text-only
inline const QString VIEW_CURSOR =
    u"cursor"_s; // TextFileView: caret document offset
inline const QString VIEW_SCROLL_X = u"scrollX"_s; // horizontal scrollbar value
inline const QString VIEW_SCROLL_Y = u"scrollY"_s; // vertical scrollbar value

// Zoom mode + factor (plus the shared scroll pair above), written and read by
// ZoomState for both scaled views — PdfFileView and ImageFileView
inline const QString VIEW_ZOOM_MODE = u"zoomMode"_s;
inline const QString VIEW_ZOOM_FACTOR = u"zoomFactor"_s;
inline const QString VIEW_ZOOM_FIT = u"fit"_s;     // ZOOM_MODE value
inline const QString VIEW_ZOOM_FIXED = u"fixed"_s; // ZOOM_MODE value

} // namespace Suzuri::WorkspaceKeys
