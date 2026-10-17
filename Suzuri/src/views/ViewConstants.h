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

#include <QColor>
#include <QPalette>
#include <QtTypes>

namespace Suzuri {

// Values tuned by hand for the parts the views are built from: the text editor
// (TextEditor), its selection handles (SelectionHandles), and the zoom control
// (ZoomControl). The views' counterpart of ui/UiConstants.h

// The floating zoom overlay on scaled document views (ZoomControl), bottom-
// right over the content. ICON_COLOR tints the - and + glyphs and
// READOUT_COLOR the readout's text: fixed colors rather than palette roles,
// since the pill floats over document content and couldn't follow the app
// palette and stay legible. BUTTON_EXTENT is a button's square footprint and
// ICON_EXTENT the glyph centered in it, the difference being padding.
// READOUT_WIDTH fits "100%"; MARGIN insets the pill from the view's corner;
// RADIUS rounds it
inline constexpr auto ZOOM_CONTROL_ICON_COLOR = QColor(255, 255, 255, 220);
inline constexpr auto ZOOM_CONTROL_READOUT_COLOR = QColor(255, 255, 255, 220);
inline constexpr int ZOOM_CONTROL_BUTTON_EXTENT = 28;
inline constexpr int ZOOM_CONTROL_ICON_EXTENT = 14;
inline constexpr int ZOOM_CONTROL_READOUT_WIDTH = 48;
inline constexpr int ZOOM_CONTROL_MARGIN = 16;
inline constexpr int ZOOM_CONTROL_RADIUS = 6;

// The editor's line-number gutter (TextEditor). ROLE is the pen the numbers are
// drawn with, muted like the status bar's text. The gutter is never narrower
// than MIN_DIGITS digits: each time it widens, the viewport narrows and the
// whole document rewraps, so a floor of 3 would put the first such jump at line
// 1000. LEFT_PADDING is the gap between the editor's edge and the numbers;
// RIGHT_PADDING the gap between the numbers and the text area (the document's
// own margin is added to that)
inline constexpr auto LINE_NUMBER_ROLE = QPalette::PlaceholderText;
inline constexpr int LINE_NUMBER_MIN_DIGITS = 1;
inline constexpr int LINE_NUMBER_LEFT_PADDING = 8;
inline constexpr int LINE_NUMBER_RIGHT_PADDING = 8;

// Space always kept between the text and the editor's sides, so text never
// runs against the pane's edge or the divider beside it. The left/right margin
// setting adds to it. TEXT_DOCUMENT_MARGIN of it is inside the viewport, and
// the rest outside
inline constexpr int TEXT_SIDE_PADDING = 20;

// The editor's current-line highlight (TextEditor): the band behind the row
// the text cursor is on. LIGHT colors it on a light page and DARK on a dark one
// (a page is dark when its color's lightness is under half). A color with an
// alpha below 255 tints the page rather than covering it. An invalid color,
// QColor(), uses ROLE from the palette instead. In Fusion that's a pale grey on
// a light page; on a dark Windows page it's the darkest shade of the Windows
// accent color, which Qt's Windows dark palette puts in AlternateBase
inline constexpr auto LINE_HIGHLIGHT_LIGHT = QColor(0, 120, 215, 46);
inline constexpr auto LINE_HIGHLIGHT_DARK = QColor(230, 70, 70, 72);
inline constexpr auto LINE_HIGHLIGHT_ROLE = QPalette::AlternateBase;

// What a search found, tinted over the text (TextEditor): this role's color
// at ALPHA of 255, light enough to read the text through. The match the
// search is on is the editor's own selection, so it is drawn as one and
// stands apart from the rest
inline constexpr auto SEARCH_MATCH_ROLE = QPalette::Highlight;
inline constexpr int SEARCH_MATCH_ALPHA = 64;

// A misspelled word's underline (TextEditor): a wave in COLOR, WIDTH thick,
// rising and falling AMPLITUDE either side of a line GAP below the text's
// baseline, each rise or fall HALF_PERIOD across. A fixed color rather than a
// palette role: no role means "error", and this red reads on a light or a dark
// text area alike
inline constexpr auto MISSPELLING_COLOR = QColor(226, 68, 68);
inline constexpr qreal MISSPELLING_WIDTH = 1.0;
inline constexpr qreal MISSPELLING_AMPLITUDE = 1.0;
inline constexpr qreal MISSPELLING_HALF_PERIOD = 2.0;
inline constexpr qreal MISSPELLING_GAP = 2.0;

// How many suggestions the editor's context menu offers for a misspelled word
inline constexpr int SPELLING_SUGGESTIONS_MAX = 5;

// The find bar across the top of a text view (FindBar). MARGIN is the space
// around its rows of controls and SPACING the gap between them, and between
// the rows. The search and replacement fields are TERM_WIDTH wide, and narrow
// as far as TERM_MIN_WIDTH in a pane too narrow for the whole row.
// BUTTON_EXTENT is a glyph button's square footprint and ICON_EXTENT the glyph
// centered in it
inline constexpr int FIND_BAR_MARGIN = 6;
inline constexpr int FIND_BAR_SPACING = 6;
inline constexpr int FIND_BAR_TERM_WIDTH = 240;
inline constexpr int FIND_BAR_TERM_MIN_WIDTH = 80;
inline constexpr int FIND_BAR_BUTTON_EXTENT = 24;
inline constexpr int FIND_BAR_ICON_EXTENT = 14;
inline constexpr auto FIND_BAR_ICON_ROLE = QPalette::WindowText;
inline constexpr auto FIND_BAR_COUNT_ROLE = QPalette::PlaceholderText;

// The editor's selection handles (SelectionHandles): a teardrop under each
// end of a selection. RADIUS is the round bulb's; STEM_HEIGHT the gap between
// the text and the top of the bulb, which the teardrop's point spans.
// HIT_RADIUS, around the bulb's center, is the area that grabs the handle:
// larger than the bulb, to be easy to catch, and a click on text within it
// takes the handle instead
inline constexpr auto SELECTION_HANDLE_FILL_ROLE = QPalette::Highlight;
inline constexpr auto SELECTION_HANDLE_BORDER_ROLE = QPalette::Base;
inline constexpr qreal SELECTION_HANDLE_BORDER_WIDTH = 1.0;
inline constexpr qreal SELECTION_HANDLE_RADIUS = 7.0;
inline constexpr qreal SELECTION_HANDLE_STEM_HEIGHT = 4.0;
inline constexpr qreal SELECTION_HANDLE_HIT_RADIUS = 14.0;

// The document's margin: the space inside the viewport between the text and
// the viewport's edges. The handles are painted into the viewport, so this is
// wide enough for a handle's bulb, its border, and a pixel for antialiasing,
// and a handle at the start or end of a line isn't cut off. Qt applies it on
// all four sides, so it is also the space above the first line and below the
// last. It is part of TEXT_SIDE_PADDING, not added to it
inline constexpr int TEXT_DOCUMENT_MARGIN =
    int(SELECTION_HANDLE_RADIUS + SELECTION_HANDLE_BORDER_WIDTH) + 1;

static_assert(TEXT_DOCUMENT_MARGIN <= TEXT_SIDE_PADDING);

} // namespace Suzuri
