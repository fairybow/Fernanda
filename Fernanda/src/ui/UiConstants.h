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
#include <QString>

namespace Fernanda::Ui {

using namespace Qt::StringLiterals;

// TODO: Layout spacing (e.g., V_LAYOUT_SPACING_1 = 12)

// Preferred (max) and floor (min) tab widths. No setters: nothing needs per-bar
// tuning. -1 uses Qt's default. TAB_HEIGHT is the height of a tab as it's
// drawn; the bar is STYLE_TAB_TOP_GAP taller, for the space the style leaves
// above every tab, so the two are set independently. -1 uses Qt's default
// height, with the gap still added
inline constexpr int MIN_TAB_WIDTH = 75;
inline constexpr int MAX_TAB_WIDTH = 225;
inline constexpr int TAB_HEIGHT = 36;

// The space between the pane's left edge and its first tab
inline constexpr int TAB_ROW_LEFT_MARGIN = 4;

// The close/unpin button on each tab (TabCloseButton). EXTENT is the button's
// square size within the tab; ICON_EXTENT is the square glyph centered inside
// it. The difference between the two is the button's padding — grow the button
// or shrink the glyph for more room around it. The glyph is clamped to the
// button, so ICON_EXTENT larger than EXTENT just fills the button rather than
// overflowing
inline constexpr int TAB_BUTTON_EXTENT = 20;
inline constexpr int TAB_BUTTON_ICON_EXTENT = 12;

// The new-tab + button (NewTabButton), laid beside the bar rather than inside a
// tab. Same EXTENT/ICON_EXTENT meaning as the close button's pair, but an
// independent knob: it sits on the TAB_HEIGHT row next to the tabs, not clamped
// inside a tab, so it can be sized on its own without disturbing the close
// button. Seeded equal to the close button's values; tune here
inline constexpr int NEW_TAB_BUTTON_EXTENT = 20;
inline constexpr int NEW_TAB_BUTTON_ICON_EXTENT = 12;

// The space between the last tab and the + button, and between the button and
// the pane's right edge. The right gap only shows once the tabs fill the row;
// until then the row's slack is already there
inline constexpr int NEW_TAB_BUTTON_LEFT_GAP = 4;
inline constexpr int NEW_TAB_BUTTON_RIGHT_GAP = 6;

// The Common Vault glyph, shared by the Common Vault drawer's header and the
// tab mark on a Common Vault file's tab. One path so swapping the Lucide icon
// is a one-place edit
inline constexpr auto COMMON_VAULT_ICON_PATH = ":/lucide/Box.svg";

// The disclosure chevrons, shared by the vault trees' branch column
// (VaultTreeView) and the Common Vault drawer's header (Drawer): Right while
// collapsed, Down while open
inline constexpr auto CHEVRON_DOWN_ICON_PATH = ":/lucide/ChevronDown.svg";
inline constexpr auto CHEVRON_RIGHT_ICON_PATH = ":/lucide/ChevronRight.svg";

// A tab's mark (PageIcon), drawn by the style ahead of the title. Bar-wide:
// Qt has no per-tab icon size. Seeded to match the close button's glyph
inline constexpr int TAB_ICON_EXTENT = 12;

// Glyph tints: the palette role each Lucide glyph is recolored from, one knob
// per glyph so any two can be matched or set apart. Folder and FolderOpen
// share a role (one folder, two states), as do the tree's ChevronDown and
// ChevronRight.
//
// Every glyph follows its widget's current color group, like everything Qt
// paints itself. A palette holds three color sets — Active, Inactive, Disabled
// — and a role resolves against whichever one the widget is in now (Disabled
// if disabled, Inactive if its window lacks focus, else Active). So these
// roles never name a group, and a palette that defines Inactive or Disabled
// colors restyles the glyphs with no change here.
//
// The Common Vault drawer's header has three knobs — chevron, Common Vault
// glyph, and the title's own pen (not a glyph, but kept beside the two it
// sits with). All seeded to ButtonText, the pen a button draws its label
// with, so the row reads as one color under any palette; set any apart here.
// The vault switcher's glyph takes ButtonText too, the pen it draws the vault
// name with
inline constexpr auto TREE_FILE_ICON_ROLE = QPalette::PlaceholderText;
inline constexpr auto TREE_FOLDER_ICON_ROLE = QPalette::PlaceholderText;
inline constexpr auto TREE_CHEVRON_ICON_ROLE = QPalette::PlaceholderText;
inline constexpr auto DRAWER_CHEVRON_ICON_ROLE = QPalette::PlaceholderText;
inline constexpr auto DRAWER_COMMON_VAULT_ICON_ROLE = QPalette::PlaceholderText;
inline constexpr auto DRAWER_TITLE_ROLE = QPalette::PlaceholderText;
inline constexpr auto VAULT_SWITCHER_ICON_ROLE = QPalette::PlaceholderText;
inline constexpr auto SETTINGS_BUTTON_ICON_ROLE = QPalette::PlaceholderText;
inline constexpr auto FOCUS_TIMER_ICON_ROLE = QPalette::PlaceholderText;
inline constexpr auto VAULT_ORDER_BUTTON_ICON_ROLE = QPalette::PlaceholderText;
inline constexpr auto TAB_CLOSE_ICON_ROLE = QPalette::WindowText;
inline constexpr auto TAB_PIN_ICON_ROLE = QPalette::PlaceholderText;
inline constexpr auto NEW_TAB_ICON_ROLE = QPalette::WindowText;
inline constexpr auto TAB_MARK_ICON_ROLE = QPalette::PlaceholderText;

inline constexpr int DEFAULT_VAULT_WINDOW_WIDTH = 800;
inline constexpr int DEFAULT_VAULT_WINDOW_HEIGHT = 600;
inline constexpr int DEFAULT_POPOUT_WINDOW_WIDTH = 600;
inline constexpr int DEFAULT_POPOUT_WINDOW_HEIGHT = 600;

inline constexpr int MAX_DRAG_PIXMAP_WIDTH = 220;

// The translucent wash painted over a drop target — an editor leaf for a tab or
// file drop, the destination folder for an entry move. One literal, two widgets
inline const QString DROP_OVERLAY_QSS =
    u"background: rgba(80, 140, 240, 60);"_s;

// How long a collapsed folder must be hovered before it opens under a drag.
// Fed to QTreeView::setAutoExpandDelay, whose own timer does the expanding
inline constexpr int TREE_AUTO_EXPAND_MS = 700;

// The labels at the right end of an item-view row (ui/widgets/RowBadges.h): a
// vault tree's file extensions, and Go to File's RECENT and COMMON.
// FONT_SCALE shrinks the row's font for the letters; H_PADDING / V_PADDING
// are the room between the letters and a label's edge; RIGHT_MARGIN insets the
// last label from the view's right edge, SPACING separates two labels, and GAP
// is the least room kept between an elided name and the first. A label drawn
// with a border has one BORDER_WIDTH wide, rounded by RADIUS
inline constexpr qreal ROW_BADGE_FONT_SCALE = 0.8;
inline constexpr int ROW_BADGE_H_PADDING = 4;
inline constexpr int ROW_BADGE_V_PADDING = 1;
inline constexpr int ROW_BADGE_RIGHT_MARGIN = 6;
inline constexpr int ROW_BADGE_SPACING = 4;
inline constexpr int ROW_BADGE_GAP = 8;
inline constexpr qreal ROW_BADGE_BORDER_WIDTH = 1.0;
inline constexpr int ROW_BADGE_RADIUS = 3;

// The file / folder glyph left of each row's name in the vault trees
// (VaultTreeItemDelegate). ENABLED false skips them outright: nothing is
// rendered and no decoration room is reserved, so rows lay out as before.
// EXTENT is the glyph's square size
inline constexpr auto TREE_ICONS_ENABLED = true;
inline constexpr int TREE_ICON_EXTENT = 14;

// The expand / collapse chevron in the vault trees' branch column
// (VaultTreeView), centered in its row's own indentation cell. Clamped to the
// cell's width and the row's height, so an over-large value fills the cell
// rather than overflowing into the name
inline constexpr int TREE_CHEVRON_EXTENT = 12;

// What the Common Vault drawer's header shows after its chevron (Drawer): the
// title alone, the Common Vault glyph then the title, or the glyph alone. A
// mode without the glyph never renders it. Icon still sets the title on the
// header — it stays the button's accessible name — and shows it as a tooltip,
// but doesn't paint it
enum class DrawerLabel
{
    Title,
    IconAndTitle,
    Icon
};

inline constexpr auto COMMON_DRAWER_LABEL = DrawerLabel::Icon;

// How the Common Vault drawer's header shows hover and press (Drawer). Row is
// the rounded wash a tree row or flat button shows, inset STYLE_ROW_INSET from
// the header's sides and DRAWER_HEADER_ROW_PADDING from its top and bottom;
// the header grows by that padding above and below, so the wash keeps
// DRAWER_HEADER_HEIGHT. Band fills the whole header, square, and adds no
// padding
enum class DrawerHeaderWash
{
    Row,
    Band
};

inline constexpr auto DRAWER_HEADER_WASH = DrawerHeaderWash::Band;
inline constexpr int DRAWER_HEADER_ROW_PADDING = 3;

// The Common Vault drawer's header row (Drawer): chevron, then the Common
// Vault glyph and/or the title, per COMMON_DRAWER_LABEL. HEIGHT is the row's
// fixed height, and so also the collapsed drawer's height; LEFT_PADDING insets
// the chevron from the header's left edge. Each gap is named for the item on
// its left: CHEVRON_SPACING follows the chevron (to the glyph, or the title
// when there's no glyph); COMMON_VAULT_ICON_SPACING follows the glyph (to the
// title — unused in Icon). The two EXTENTs are the glyphs' square sizes, each
// clamped to HEIGHT; a clamp never shifts what follows. A fixed height doesn't
// track the font: a larger UI font clips the title rather than growing the row
inline constexpr int DRAWER_HEADER_HEIGHT = 26;
inline constexpr int DRAWER_HEADER_LEFT_PADDING = 8;
inline constexpr int DRAWER_CHEVRON_SPACING = 6;
inline constexpr int DRAWER_COMMON_VAULT_ICON_SPACING = 6;
inline constexpr int DRAWER_CHEVRON_EXTENT = 12;
inline constexpr int DRAWER_COMMON_VAULT_ICON_EXTENT = 12;

// The vault switcher's button (VaultSwitcher): ChevronsUpDown, then the vault
// name. The four knobs mean what the drawer header's do. LEFT_PADDING is
// measured from the button's own edge, frame included; 4 is where the style's
// own label would put the glyph
inline constexpr int VAULT_SWITCHER_HEIGHT = 28;
inline constexpr int VAULT_SWITCHER_LEFT_PADDING = 8;
inline constexpr int VAULT_SWITCHER_ICON_SPACING = 6;
inline constexpr int VAULT_SWITCHER_ICON_EXTENT = 14;

// The settings gear right of the vault switcher (SettingsButton), Obsidian's
// vault profile row. EXTENT/ICON_EXTENT mean what the tab buttons' pair means;
// EXTENT is seeded to VAULT_SWITCHER_HEIGHT so the gear is a square the row's
// own height
inline constexpr int SETTINGS_BUTTON_EXTENT = VAULT_SWITCHER_HEIGHT;
inline constexpr int SETTINGS_BUTTON_ICON_EXTENT = 16;

// The focus timer's button left of the gear (FocusTimerButton), the gear's
// size. Its glyph is smaller, to sit inside a ring showing what is left of
// the period: RING_INSET is the ring's distance from the button's edge and
// RING_WIDTH its stroke. The ring is drawn in RING_ROLE while a period runs
// and once one has run out, PAUSED_RING_ROLE while paused, and its track in
// the style's divider. The glyph's tint is FOCUS_TIMER_ICON_ROLE, with the
// other glyphs' roles
inline constexpr int FOCUS_TIMER_BUTTON_EXTENT = SETTINGS_BUTTON_EXTENT;
inline constexpr int FOCUS_TIMER_BUTTON_ICON_EXTENT = 12;
inline constexpr qreal FOCUS_TIMER_RING_INSET = 3.0;
inline constexpr qreal FOCUS_TIMER_RING_WIDTH = 1.5;
inline constexpr auto FOCUS_TIMER_RING_ROLE = QPalette::Accent;
inline constexpr auto FOCUS_TIMER_PAUSED_RING_ROLE = QPalette::PlaceholderText;
inline constexpr auto FOCUS_TIMER_FOCUS_ICON_PATH = ":/lucide/Timer.svg";
inline constexpr auto FOCUS_TIMER_BREAK_ICON_PATH = ":/lucide/Coffee.svg";

// The space around the sidebar's footer, the row holding the vault switcher
// and the gear (Sidebar): from the sidebar's left edge, from the line above
// it, from the sidebar's right edge, and from the window's bottom edge
inline constexpr int SIDEBAR_FOOTER_LEFT_MARGIN = 4;
inline constexpr int SIDEBAR_FOOTER_TOP_MARGIN = 4;
inline constexpr int SIDEBAR_FOOTER_RIGHT_MARGIN = 4;
inline constexpr int SIDEBAR_FOOTER_BOTTOM_MARGIN = 0;

// The settings dialog (SettingsDialog): its opening size; the navigation
// column's fixed width, and the margin around (and gap within) that column;
// the margin around a page's rows. GROUP_TITLE_ROLE tints the nav's group
// titles ("Options"). An item's foreground is a fixed color, not a role, so
// the dialog reads this when it adds a title and again on every PaletteChange
inline constexpr int SETTINGS_DIALOG_WIDTH = 860;
inline constexpr int SETTINGS_DIALOG_HEIGHT = 600;
inline constexpr int SETTINGS_NAV_WIDTH = 200;
inline constexpr int SETTINGS_NAV_MARGIN = 12;
inline constexpr int SETTINGS_PAGE_MARGIN = 24;
inline constexpr auto SETTINGS_NAV_GROUP_TITLE_ROLE = QPalette::PlaceholderText;

// The vault picker (ManageVaults): its size, which is fixed, unless its
// contents need more
inline constexpr int MANAGE_VAULTS_WIDTH = 560;
inline constexpr int MANAGE_VAULTS_HEIGHT = 380;

// The glyph beside a vault whose folder is missing, in the vault picker's
// list. EXTENT is its square size, the vault trees' icon size. Tinted with ROLE
// from the Disabled group, as the row's greyed name is drawn
inline constexpr int MANAGE_VAULTS_MISSING_ICON_EXTENT = TREE_ICON_EXTENT;
inline constexpr auto MANAGE_VAULTS_MISSING_ICON_ROLE = QPalette::Text;

// The order button over the vault picker's list (VaultOrderButton), level with
// the list's title. EXTENT/ICON_EXTENT mean what the tab buttons' pair means
inline constexpr int VAULT_ORDER_BUTTON_EXTENT = 24;
inline constexpr int VAULT_ORDER_BUTTON_ICON_EXTENT = 16;

// The licenses dialog (LicensesDialog): its opening size, and the width of the
// list of works down its left
inline constexpr int LICENSES_DIALOG_WIDTH = 860;
inline constexpr int LICENSES_DIALOG_HEIGHT = 600;
inline constexpr int LICENSES_LIST_WIDTH = 200;

// One row on a settings page (SettingRow): V_PADDING above and below its
// content; INFO_SPACING between the name and its description;
// CONTROL_SPACING the least gap between the text and the control. The
// description is set smaller (FONT_SCALE) in a muted role. The line above each
// row (SettingRule) is RULE_WIDTH tall, filled with RULE_ROLE. A section
// heading (SettingHeading) gets TOP_PADDING above it and BOTTOM_PADDING before
// its first row
inline constexpr int SETTING_ROW_V_PADDING = 12;
inline constexpr int SETTING_ROW_INFO_SPACING = 4;
inline constexpr int SETTING_ROW_CONTROL_SPACING = 24;
inline constexpr qreal SETTING_DESCRIPTION_FONT_SCALE = 0.9;
inline constexpr auto SETTING_DESCRIPTION_ROLE = QPalette::PlaceholderText;
inline constexpr auto SETTING_ROW_RULE_ROLE = QPalette::Mid;
inline constexpr int SETTING_ROW_RULE_WIDTH = 1;
inline constexpr int SETTING_HEADING_TOP_PADDING = 16;
inline constexpr int SETTING_HEADING_BOTTOM_PADDING = 8;

// The on/off switch (ToggleSwitch), Material 3-style. WIDTH x HEIGHT is the
// pill; OUTLINE_WIDTH the off track's outline, which fades into the fill as it
// turns on. The thumb grows from OFF_THUMB_DIAMETER to ON_THUMB_DIAMETER as it
// slides across, over ANIMATION_MS. Roles: on, the track is ON_TRACK_ROLE (the
// accent) with an ON_THUMB_ROLE thumb; off, the track is OFF_TRACK_ROLE (the
// window's own color) outlined in OFF_OUTLINE_ROLE, with an OFF_THUMB_ROLE
// thumb. Every color in between is a blend of the two states
inline constexpr int TOGGLE_WIDTH = 40;
inline constexpr int TOGGLE_HEIGHT = 22;
inline constexpr qreal TOGGLE_OUTLINE_WIDTH = 1.5;
inline constexpr qreal TOGGLE_OFF_THUMB_DIAMETER = 10.0;
inline constexpr qreal TOGGLE_ON_THUMB_DIAMETER = 16.0;
inline constexpr int TOGGLE_ANIMATION_MS = 150;
inline constexpr auto TOGGLE_ON_TRACK_ROLE = QPalette::Accent;
inline constexpr auto TOGGLE_ON_THUMB_ROLE = QPalette::HighlightedText;
inline constexpr auto TOGGLE_OFF_TRACK_ROLE = QPalette::Window;
inline constexpr auto TOGGLE_OFF_OUTLINE_ROLE = QPalette::Mid;
inline constexpr auto TOGGLE_OFF_THUMB_ROLE = QPalette::Mid;

// How faded a disabled switch is drawn (1.0 is not at all)
inline constexpr qreal TOGGLE_DISABLED_OPACITY = 0.4;

// The switch's keyboard-focus ring: a pill outline FOCUS_MARGIN outside the
// track on every side (the widget is that much larger than the pill, so the
// ring isn't clipped), RING_WIDTH thick, in RING_ROLE
inline constexpr int TOGGLE_FOCUS_MARGIN = 3;
inline constexpr qreal TOGGLE_FOCUS_RING_WIDTH = 1.5;
inline constexpr auto TOGGLE_FOCUS_RING_ROLE = QPalette::WindowText;

// The slider with a value readout (DisplaySlider), as on the settings page's
// Font size row: MIN_WIDTH is the slider's least width, READOUT_SPACING the
// gap between it and the number, and READOUT_DIGITS the fewest digits the
// readout makes room for, so every slider in the settings is one length
inline constexpr int DISPLAY_SLIDER_MIN_WIDTH = 160;
inline constexpr int DISPLAY_SLIDER_READOUT_SPACING = 8;
inline constexpr int DISPLAY_SLIDER_READOUT_DIGITS = 3;

// The font family picker (FontFamilyBox) is as wide as this many characters,
// however long the longest installed family name is
inline constexpr int FONT_FAMILY_BOX_MIN_CHARS = 24;

// The Go to File switcher (FileSwitcher), placed top-center over its host
// window. WIDTH is clamped to the host's width; TOP_OFFSET is the gap between
// the host's top edge and the switcher's. MARGIN surrounds its contents and
// SPACING separates the field, the hint, and the list
inline constexpr int FILE_SWITCHER_WIDTH = 560;
inline constexpr int FILE_SWITCHER_HEIGHT = 360;
inline constexpr int FILE_SWITCHER_TOP_OFFSET = 80;
inline constexpr int FILE_SWITCHER_MARGIN = 12;
inline constexpr int FILE_SWITCHER_SPACING = 8;

// The window status bar's items (WordCounter, CursorPosition). TEXT_ROLE is the
// pen their text is drawn with, muted like the sidebar's chrome. ITEM_H_PADDING
// is the space kept left and right of each item's text, so two items side by
// side sit twice that apart
inline constexpr auto STATUS_BAR_TEXT_ROLE = QPalette::PlaceholderText;
inline constexpr int STATUS_BAR_ITEM_H_PADDING = 8;

// The status bar's update notice (UpdateNotice), left of the other items.
// EXTENT/ICON_EXTENT mean what the tab buttons' pair means. The glyph is in the
// accent color, to be noticed
inline constexpr auto UPDATE_NOTICE_ICON_PATH = ":/lucide/ArrowDownToLine.svg";
inline constexpr int UPDATE_NOTICE_EXTENT = 20;
inline constexpr int UPDATE_NOTICE_ICON_EXTENT = 14;
inline constexpr auto UPDATE_NOTICE_ICON_ROLE = QPalette::Accent;

// A checkable GlyphButton while it's checked: its panel filled with
// CHECKED_PANEL_ROLE, and its glyph in CHECKED_ICON_ROLE in place of its own
// tint, the pen that reads on the accent (as a toggle switch's thumb does)
inline constexpr auto GLYPH_BUTTON_CHECKED_PANEL_ROLE = QPalette::Accent;
inline constexpr auto GLYPH_BUTTON_CHECKED_ICON_ROLE =
    QPalette::HighlightedText;

// The status bar's Git button (GitButton), at the bar's left, shown while the
// vault is in a Git repository. EXTENT/ICON_EXTENT mean what the update
// notice's pair means. Filled with the accent while the Git view is open
inline constexpr int GIT_BUTTON_EXTENT = 20;
inline constexpr int GIT_BUTTON_ICON_EXTENT = 14;
inline constexpr auto GIT_BUTTON_ICON_ROLE = QPalette::PlaceholderText;

// The Git view (GitView). CHANGES_WIDTH is the first width of its left column,
// the list of changed files; the rest goes to the right. HEADER_H_MARGIN and
// HEADER_V_MARGIN surround the count of changed files above the list.
// MUTED_TEXT_ROLE is the pen of the line on the right when there's nothing to
// show. Each row of the list has its change's glyph, CHANGE_ICON_EXTENT
// square, against the list's right edge, in its own tint
inline constexpr int GIT_VIEW_CHANGES_WIDTH = 280;
inline constexpr int GIT_VIEW_HEADER_H_MARGIN = 6;
inline constexpr int GIT_VIEW_HEADER_V_MARGIN = 6;
inline constexpr auto GIT_VIEW_MUTED_TEXT_ROLE = QPalette::PlaceholderText;
inline constexpr int GIT_CHANGE_ICON_EXTENT = 14;
inline constexpr auto GIT_ADDED_ICON_ROLE = QPalette::PlaceholderText;
inline constexpr auto GIT_MODIFIED_ICON_ROLE = QPalette::PlaceholderText;
inline constexpr auto GIT_DELETED_ICON_ROLE = QPalette::PlaceholderText;
inline constexpr auto GIT_RENAMED_ICON_ROLE = QPalette::PlaceholderText;
inline constexpr auto GIT_CONFLICTED_ICON_ROLE = QPalette::PlaceholderText;

// The Git view's commit box, below its list: COMMIT_MARGIN around it and
// COMMIT_SPACING between the summary, the description, and the button. The
// description is DESCRIPTION_LINES lines of its font tall
inline constexpr int GIT_VIEW_COMMIT_MARGIN = 6;
inline constexpr int GIT_VIEW_COMMIT_SPACING = 6;
inline constexpr int GIT_VIEW_DESCRIPTION_LINES = 4;

// The Git view's top bar (GitTopBar) and its sections (GitBarItem), each
// HEIGHT tall and ITEM_WIDTH wide, narrowing to ITEM_MIN_WIDTH when there
// isn't room. A section is ITEM_PADDING from its edges, its glyph ICON_EXTENT
// square and ICON_SPACING from the text, its chevron CHEVRON_EXTENT square.
// The caption above the name is the font at CAPTION_SCALE, in CAPTION_ROLE
inline constexpr int GIT_BAR_HEIGHT = 48;
inline constexpr int GIT_BAR_ITEM_WIDTH = 240;
inline constexpr int GIT_BAR_ITEM_MIN_WIDTH = 140;
inline constexpr int GIT_BAR_ITEM_PADDING = 12;
inline constexpr int GIT_BAR_ICON_EXTENT = 16;
inline constexpr int GIT_BAR_ICON_SPACING = 10;
inline constexpr int GIT_BAR_CHEVRON_EXTENT = 12;
inline constexpr qreal GIT_BAR_CAPTION_SCALE = 0.85;
inline constexpr auto GIT_BAR_ICON_ROLE = QPalette::PlaceholderText;
inline constexpr auto GIT_BAR_CAPTION_ROLE = QPalette::PlaceholderText;

// The list of branches the top bar drops (GitBranchPopup): at least
// MIN_WIDTH wide, HEIGHT tall, with MARGIN around and between its parts
inline constexpr int GIT_BRANCH_POPUP_MIN_WIDTH = 300;
inline constexpr int GIT_BRANCH_POPUP_HEIGHT = 360;
inline constexpr int GIT_BRANCH_POPUP_MARGIN = 6;

// The Git view's diff (GitDiffView). An added line is tinted ADDED_BACKGROUND
// and a removed one REMOVED_BACKGROUND, across the text and the gutter. Each
// has an alpha, so it tints the page rather than covering it, on a light page
// or a dark one. HEADER_ROLE is the pen of each hunk's @@ line, and
// NUMBER_ROLE the gutter's. The gutter is GUTTER_PADDING on each side of its
// two columns of numbers, which are GUTTER_SPACING apart, and TEXT_PADDING
// more on its right, between the numbers and the text
inline constexpr auto GIT_DIFF_ADDED_BACKGROUND = QColor(46, 160, 67, 56);
inline constexpr auto GIT_DIFF_REMOVED_BACKGROUND = QColor(248, 81, 73, 56);
inline constexpr auto GIT_DIFF_HEADER_ROLE = QPalette::PlaceholderText;
inline constexpr auto GIT_DIFF_NUMBER_ROLE = QPalette::PlaceholderText;
inline constexpr int GIT_DIFF_GUTTER_PADDING = 8;
inline constexpr int GIT_DIFF_GUTTER_SPACING = 8;
inline constexpr int GIT_DIFF_TEXT_PADDING = 8;

// Fernanda's style (Style). Shades the palette has no role for are its text
// color at an alpha, drawn over whatever is behind them: HOVER under the
// pointer, PRESSED held down (and a checked tool button), SELECTED for a
// selected row or menu item (SELECTED_HOVER for one that's both), DIVIDER for
// the one-pixel lines (splitter handles, between tabs), CONTROL_BORDER around
// a button, combo box, field, framed list, or menu. MUTED_TEXT is the title of
// a tab that isn't current, and a combo box's arrow
inline constexpr qreal STYLE_HOVER_ALPHA = 0.06;
inline constexpr qreal STYLE_PRESSED_ALPHA = 0.12;
inline constexpr qreal STYLE_SELECTED_ALPHA = 0.10;
inline constexpr qreal STYLE_SELECTED_HOVER_ALPHA = 0.14;
inline constexpr qreal STYLE_DIVIDER_ALPHA = 0.12;
inline constexpr qreal STYLE_CONTROL_BORDER_ALPHA = 0.18;
inline constexpr qreal STYLE_MUTED_TEXT_ALPHA = 0.6;

// Rounded corners (Style): a tool button's wash (and a hovered tab's), a push
// button, combo box, field, or framed list, an item-view row's wash (and a
// menu item's), and the current tab's top
inline constexpr int STYLE_TOOL_BUTTON_RADIUS = 4;
inline constexpr int STYLE_CONTROL_RADIUS = 6;
inline constexpr int STYLE_ROW_RADIUS = 4;
inline constexpr int STYLE_TAB_RADIUS = 6;

// In a light palette, the window's color is moved this far toward the page's
// (Style): 0 keeps the platform's own, 1 is the page's. A dark palette is left
// as it is
inline constexpr qreal STYLE_WINDOW_TOWARD_BASE = 0.6;

// Tabs (Style). TAB_TOP_GAP is left above every tab, so the current one's
// rounded top shows against the bar (the bar is TAB_HEIGHT plus this). A
// hovered tab's wash is inset TAB_HOVER_INSET, and the divider between tabs
// stops TAB_DIVIDER_MARGIN short of the tab's top and bottom
inline constexpr int STYLE_TAB_TOP_GAP = 4;
inline constexpr int STYLE_TAB_HOVER_INSET = 3;
inline constexpr int STYLE_TAB_DIVIDER_MARGIN = 8;

// Item-view rows (Style). Every row's wash, in a tree or a list, is inset
// ROW_INSET from the view's sides (the drawer header's Row wash is too). A
// tree's rows are TREE_ROW_V_PADDING taller above and below their text, and
// keep it TREE_ROW_TEXT_MARGIN (plus one) from their sides
inline constexpr int STYLE_ROW_INSET = 4;
inline constexpr int STYLE_TREE_ROW_V_PADDING = 3;
inline constexpr int STYLE_TREE_ROW_TEXT_MARGIN = 6;

// Lists, any item view but a tree (Style): Go to File, the settings
// navigation. Their rows are LIST_ROW_V_PADDING taller above and below their
// text and keep it LIST_ROW_TEXT_MARGIN (plus one) from their sides; a framed
// list's rows are LIST_FRAME_PADDING inside its border. The selected row is
// marked by a rounded bar in the accent, LIST_INDICATOR_WIDTH by
// LIST_INDICATOR_HEIGHT, LIST_INDICATOR_INSET inside the row's wash
inline constexpr int STYLE_LIST_ROW_V_PADDING = 9;
inline constexpr int STYLE_LIST_ROW_TEXT_MARGIN = 12;
inline constexpr int STYLE_LIST_FRAME_PADDING = 4;
inline constexpr int STYLE_LIST_INDICATOR_WIDTH = 3;
inline constexpr int STYLE_LIST_INDICATOR_HEIGHT = 16;
inline constexpr int STYLE_LIST_INDICATOR_INSET = 1;

// Text fields and combo boxes (Style). A framed field keeps its text
// FIELD_H_PADDING from its sides and is FIELD_V_PADDING taller above and below.
// A combo box has FIELD_H_PADDING between its text and its arrow, and the arrow
// is inset COMBO_ARROW_INSET within its sub-rect. Its drop-down list, drawn as
// a menu, has its window COMBO_POPUP_OVERHANG wider than the box on each side:
// the menu's shadow margin, plus enough that the rows' text starts where the
// box's does
inline constexpr int STYLE_FIELD_H_PADDING = 6;
inline constexpr int STYLE_FIELD_V_PADDING = 2;
inline constexpr int STYLE_COMBO_ARROW_INSET = 2;
inline constexpr int STYLE_COMBO_POPUP_OVERHANG = 9;

// Menus (Style). The panel is rounded to MENU_RADIUS inside a margin of
// MENU_SHADOW pixels that holds its shadow, each step of which is up to
// MENU_SHADOW_ALPHA black. Items sit MENU_H_PADDING from the panel's sides
// and MENU_V_PADDING from its top and bottom; each is MENU_ITEM_V_PADDING
// taller above and below its text, and its wash is inset MENU_ITEM_INSET from
// its sides. Fusion leaves room left of an item's text for a check mark or
// icon; in a menu with neither, MENU_TEXT_TRIM pixels of that room are taken
// back, from the text's left and the menu's width
inline constexpr int STYLE_MENU_RADIUS = 8;
inline constexpr int STYLE_MENU_SHADOW = 6;
inline constexpr qreal STYLE_MENU_SHADOW_ALPHA = 0.05;
inline constexpr int STYLE_MENU_H_PADDING = 2;
inline constexpr int STYLE_MENU_V_PADDING = 4;
inline constexpr int STYLE_MENU_ITEM_V_PADDING = 2;
inline constexpr int STYLE_MENU_ITEM_INSET = 4;
inline constexpr int STYLE_MENU_TEXT_TRIM = 12;

// Sliders (Style): a track SLIDER_TRACK_HEIGHT tall, at SLIDER_TRACK_ALPHA
// where it isn't filled with the accent, and a round handle
// SLIDER_HANDLE_DIAMETER across
inline constexpr int STYLE_SLIDER_TRACK_HEIGHT = 4;
inline constexpr qreal STYLE_SLIDER_TRACK_ALPHA = 0.15;
inline constexpr int STYLE_SLIDER_HANDLE_DIAMETER = 14;

// Scroll bars (Style): SCROLL_BAR_EXTENT across, with a handle no shorter
// than SCROLL_BAR_HANDLE_MIN, inset SCROLL_BAR_HANDLE_INSET from the bar's
// edges. The handle is the text color at SCROLL_BAR_ALPHA, or HOVER_ and
// PRESSED_ while the pointer is on it or dragging it
inline constexpr int STYLE_SCROLL_BAR_EXTENT = 10;
inline constexpr int STYLE_SCROLL_BAR_HANDLE_MIN = 32;
inline constexpr qreal STYLE_SCROLL_BAR_HANDLE_INSET = 2.0;
inline constexpr qreal STYLE_SCROLL_BAR_ALPHA = 0.22;
inline constexpr qreal STYLE_SCROLL_BAR_HOVER_ALPHA = 0.35;
inline constexpr qreal STYLE_SCROLL_BAR_PRESSED_ALPHA = 0.45;

} // namespace Fernanda::Ui
