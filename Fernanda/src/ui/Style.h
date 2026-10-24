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

#include <QAbstractItemView>
#include <QAbstractScrollArea>
#include <QBrush>
#include <QColor>
#include <QComboBox>
#include <QFrame>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QPen>
#include <QPointF>
#include <QProxyStyle>
#include <QRect>
#include <QRectF>
#include <QSize>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleOption>
#include <QTabBar>
#include <QTreeView>
#include <QWidget>
#include <QtMinMax>

#include "ui/UiConstants.h"

namespace Fernanda::Ui {

using namespace Qt::StringLiterals;

// Fernanda's look: Fusion, with Fernanda's own refinements on top. App installs
// one for the whole application before any window exists.
//
// Fusion rather than each platform's own style, for two reasons. Fernanda looks
// the same on every OS. And Fusion draws every standard control from the
// palette alone, which is what follows the OS's light or dark mode (on Windows
// 10 too, where the native style has no dark mode). The platform styles draw
// parts of themselves in colors of their own.
//
// The proxy is where Fernanda changes how Fusion draws: flatter, rounder, and
// quieter. Every color still comes from the palette. Where Fernanda wants a
// shade the palette has no role for (a hover, a divider), it's a palette color
// at a low alpha (wash_), so it lands right over any background and in light
// or dark alike. What's overridden:
//
// - Tool buttons (the tab close and pin, +, the gear, the vault switcher, the
//   find bar's buttons) draw nothing at rest, and a rounded wash on hover and
//   press
// - Tabs are flat. The current tab is the page's own color with rounded top
//   corners, so it reads as part of the page below it; the others are text on
//   the bar, with a wash on hover and a short divider between them
// - Splitter handles are one-pixel dividers. A one-pixel handle is QSplitter's
//   "tiny" mode, which keeps a wider area around it to grab
// - Scroll bars are thin, with no arrows: a rounded handle on the scrolled
//   content's own background
// - Item-view rows (the file trees, Go to File, the settings nav) get a rounded
//   wash for hover and selection rather than the accent, and keep their text
//   and icon colors. In a tree it reaches back over the row's chevron. A
//   list's rows are taller, and its selected row is marked in the accent at
//   its left end. No dotted focus rectangle
// - A framed item view is a rounded panel with its rows inset from the border
// - Push buttons, combo boxes, and line edits are rounded and flat, with a
//   hairline border. A default button takes the accent. A focused field's
//   border does too, and a field's text has room around it
// - Sliders are a thin rounded track, filled with the accent up to a round
//   handle
// - Menus are rounded panels in the page's color, with a hairline border and
//   a soft shadow, and their items and the menu bar's are highlighted with
//   the rounded wash. A combo box's drop-down list is drawn the same way
// - In a light palette, the window's color is moved toward the page's
//   (polish), so the sidebar, tab bar, and dialogs are lighter than the
//   platform's grey
// - The status bar's size grip isn't drawn (it still works)
//
// Everything else is Fusion's
class Style : public QProxyStyle
{
public:
    Style()
        : QProxyStyle(QStyleFactory::create(u"Fusion"_s))
    {
    }

    // A flat button's hover or press shade, for a widget that fills its own
    // shape with it rather than having the style draw a button's rounded one
    // (the drawer header's Band)
    [[nodiscard]] static QColor
    buttonWash(const QPalette& palette, bool pressed)
    {
        return wash_(
            palette,
            pressed ? STYLE_PRESSED_ALPHA : STYLE_HOVER_ALPHA);
    }

    // Where an item view paints the wash of the item whose cell is itemRect:
    // the hover and selection shade. Public so a view can take the wash as
    // the extent of its row, as VaultTreeView does
    [[nodiscard]] static QRectF
    itemWashRect(const QRect& itemRect, const QWidget* widget)
    {
        return washRect_(itemRowRect_(itemRect, widget));
    }

    // The one-pixel lines between things: splitter handles, the short marks
    // between tabs, and the Common Vault drawer's bottom edge when it's
    // collapsed. Public so a widget that draws its own line matches these
    [[nodiscard]] static QColor divider(const QPalette& palette)
    {
        return wash_(palette, STYLE_DIVIDER_ALPHA);
    }

    using QProxyStyle::polish;
    using QProxyStyle::unpolish;

    // In a light palette, the window's color is lightened toward the page's,
    // as the platforms' greys are heavier than Fernanda wants beside white text
    // pages. A dark palette is left as it is: there, moving the window toward
    // the page would only blur the line between the sidebar and the text. Qt
    // runs the application's palette through this whenever it builds it,
    // including after an OS light or dark switch, so every surface that uses
    // the window's color (the sidebar, the tab bar, dialogs) follows
    void polish(QPalette& palette) override
    {
        QProxyStyle::polish(palette);

        if (palette.color(QPalette::Window).lightness() < 128) {
            return;
        }

        for (auto group :
             { QPalette::Active, QPalette::Inactive, QPalette::Disabled }) {
            palette.setColor(
                group,
                QPalette::Window,
                blend_(
                    palette.color(group, QPalette::Window),
                    palette.color(group, QPalette::Base),
                    STYLE_WINDOW_TOWARD_BASE));
        }
    }

    // Fusion asks for hover events on buttons, combo boxes, sliders, and scroll
    // bars, but not on tab bars or item views, whose hover washes need them.
    //
    // A menu is drawn with rounded corners and a soft shadow (drawMenuPanel_),
    // so its window has to be see-through around them, and the system's own
    // square shadow is turned off. Both are set here, before the menu's window
    // is first made. On Windows a window is see-through only if it's also
    // frameless (Qt::WA_TranslucentBackground), and a popup doesn't count as
    // frameless unless it says so, so the flag is set first.
    //
    // A combo box's drop-down list gets the same (isComboPopup_): Qt fills
    // its window with the menu panel, which an opaque, square window would
    // show inside a second edge
    void polish(QWidget* widget) override
    {
        QProxyStyle::polish(widget);
        setHoverTracked_(widget, true);

        // An item view's frame paints its background, rounded
        // (drawItemViewFrame_), so the view's square viewport mustn't paint
        // over it. A view without a frame shows what's behind it, which for
        // the file trees is the sidebar's own color
        if (auto* view = qobject_cast<QAbstractItemView*>(widget)) {
            view->viewport()->setAutoFillBackground(false);
        }

        if (qobject_cast<QMenu*>(widget) || isComboPopup_(widget)) {
            widget->setWindowFlags(widget->windowFlags() | MENU_WINDOW_FLAGS_);
            widget->setAttribute(Qt::WA_TranslucentBackground, true);
        }
    }

    void unpolish(QWidget* widget) override
    {
        if (qobject_cast<QMenu*>(widget) || isComboPopup_(widget)) {
            widget->setAttribute(Qt::WA_TranslucentBackground, false);
            widget->setWindowFlags(widget->windowFlags() & ~MENU_WINDOW_FLAGS_);
        }

        setHoverTracked_(widget, false);
        QProxyStyle::unpolish(widget);
    }

    int pixelMetric(
        PixelMetric metric,
        const QStyleOption* option = nullptr,
        const QWidget* widget = nullptr) const override
    {
        switch (metric) {
        case PM_SplitterWidth:
            return 1;
        case PM_FocusFrameHMargin:
            // An item's text sits this far (plus one) from its row's edges,
            // inside the wash, which is inset STYLE_ROW_INSET. A list's rows
            // stand alone, so their text has more room than a tree's, whose
            // icons already set it off
            if (qobject_cast<const QTreeView*>(widget)) {
                return STYLE_TREE_ROW_TEXT_MARGIN;
            }
            if (qobject_cast<const QAbstractItemView*>(widget)) {
                return STYLE_LIST_ROW_TEXT_MARGIN;
            }
            break;
        case PM_DefaultFrameWidth:
            // A drop-down list's frame is a menu's margin: the shadow and the
            // border inside it, which keep its rows inside the panel
            if (isComboPopup_(widget)) {
                return STYLE_MENU_SHADOW + 1;
            }

            // A framed item view's rows sit inside its rounded panel, not
            // against the border
            if (qobject_cast<const QAbstractItemView*>(widget)) {
                return STYLE_LIST_FRAME_PADDING;
            }
            break;
        case PM_MenuPanelWidth:
            // The shadow's margin and the border inside it
            return STYLE_MENU_SHADOW + 1;
        case PM_MenuHMargin:
            return STYLE_MENU_H_PADDING;
        case PM_MenuVMargin:
            return STYLE_MENU_V_PADDING;
        case PM_ScrollBarExtent:
            return STYLE_SCROLL_BAR_EXTENT;
        case PM_ScrollBarSliderMin:
            return STYLE_SCROLL_BAR_HANDLE_MIN;
        case PM_ButtonShiftHorizontal:
        case PM_ButtonShiftVertical:
        case PM_TabBarTabShiftHorizontal:
        case PM_TabBarTabShiftVertical:
        case PM_TabBarBaseOverlap:
            return 0;
        default:
            break;
        }

        return QProxyStyle::pixelMetric(metric, option, widget);
    }

    int styleHint(
        StyleHint hint,
        const QStyleOption* option = nullptr,
        const QWidget* widget = nullptr,
        QStyleHintReturn* returnData = nullptr) const override
    {
        switch (hint) {
        case SH_ItemView_ShowDecorationSelected:
            // A row's wash is drawn once, with its item (CE_ItemViewItem),
            // reaching back over a tree row's chevron itself
            return 0;
        default:
            return QProxyStyle::styleHint(hint, option, widget, returnData);
        }
    }

    QSize sizeFromContents(
        ContentsType type,
        const QStyleOption* option,
        const QSize& size,
        const QWidget* widget = nullptr) const override
    {
        auto result = QProxyStyle::sizeFromContents(type, option, size, widget);

        // Rows, fields, and menu items with room around their text: for a
        // row's or item's wash to sit in, and for a field's rounded border
        if (type == CT_ItemViewItem) {
            result.rheight() += 2 * (qobject_cast<const QTreeView*>(widget)
                                         ? STYLE_TREE_ROW_V_PADDING
                                         : STYLE_LIST_ROW_V_PADDING);
        } else if (type == CT_ComboBox) {
            // Qt sizes a combo box so its longest text exactly fills the room
            // before the arrow. Under Fusion a few pixels are left over, from
            // QCommonStyle's allowance for the arrow, which follows the scroll
            // bar's width; Fernanda's thinner scroll bars take them away, and
            // text drawn a fraction wider than measured (as at 125% scale)
            // loses its end. The text gets FIELD_H_PADDING after it instead,
            // as it has before it
            result.rwidth() += STYLE_FIELD_H_PADDING;
        } else if (type == CT_LineEdit) {
            // A field with room around its text (see SE_LineEditContents)
            result +=
                QSize(2 * STYLE_FIELD_H_PADDING, 2 * STYLE_FIELD_V_PADDING);
        } else if (type == CT_MenuItem) {
            auto* item = qstyleoption_cast<const QStyleOptionMenuItem*>(option);
            if (item && item->menuItemType != QStyleOptionMenuItem::Separator) {
                result.rheight() += 2 * STYLE_MENU_ITEM_V_PADDING;

                if (trimsMenuText_(item)) {
                    result.rwidth() -= STYLE_MENU_TEXT_TRIM;
                }
            }
        }

        return result;
    }

    QRect subElementRect(
        SubElement element,
        const QStyleOption* option,
        const QWidget* widget = nullptr) const override
    {
        auto rect = QProxyStyle::subElementRect(element, option, widget);

        switch (element) {
        case SE_LineEditContents:
            // The text keeps clear of the field's rounded border
            if (auto* frame =
                    qstyleoption_cast<const QStyleOptionFrame*>(option);
                frame && frame->lineWidth > 0) {
                return rect.adjusted(
                    STYLE_FIELD_H_PADDING,
                    0,
                    -STYLE_FIELD_H_PADDING,
                    0);
            }
            return rect;
        case SE_TabBarTabLeftButton:
        case SE_TabBarTabRightButton:
            // A tab's buttons are centered in the part of the tab that's
            // drawn, below the gap at its top (see drawTabShape_). Its text
            // and icon are too, by the label's own rect (CE_TabBarTabLabel)
            return rect.translated(0, STYLE_TAB_TOP_GAP / 2);
        case SE_ItemViewItemText:
            // In a list, an item's text keeps the list's margin from the row's
            // edges, and a tree's from an icon before it, as in a tree. Left to
            // the list's margin there too, the text sits well away from its
            // icon, which itself is where the other rows' text begins
            if (auto* item =
                    qstyleoption_cast<const QStyleOptionViewItem*>(option);
                item &&
                (item->features & QStyleOptionViewItem::HasDecoration) &&
                qobject_cast<const QAbstractItemView*>(widget) &&
                !qobject_cast<const QTreeView*>(widget)) {
                return rect.adjusted(
                    STYLE_TREE_ROW_TEXT_MARGIN - STYLE_LIST_ROW_TEXT_MARGIN,
                    0,
                    0,
                    0);
            }
            return rect;
        default:
            return rect;
        }
    }

    QRect subControlRect(
        ComplexControl control,
        const QStyleOptionComplex* option,
        SubControl subControl,
        const QWidget* widget = nullptr) const override
    {
        // A drop-down list's window reaches past its combo box on both sides
        // (see STYLE_COMBO_POPUP_OVERHANG), so its rows' text lines up with
        // the box's and has the box's room
        if (control == CC_ComboBox && subControl == SC_ComboBoxListBoxPopup) {
            auto rect = QProxyStyle::subControlRect(
                control,
                option,
                subControl,
                widget);

            if (auto* combo =
                    qstyleoption_cast<const QStyleOptionComboBox*>(option);
                combo && !combo->editable) {
                return rect.adjusted(
                    -STYLE_COMBO_POPUP_OVERHANG,
                    0,
                    STYLE_COMBO_POPUP_OVERHANG,
                    0);
            }

            return rect;
        }

        if (control == CC_ScrollBar) {
            if (auto* bar =
                    qstyleoption_cast<const QStyleOptionSlider*>(option)) {
                return scrollBarRect_(bar, subControl);
            }
        }

        return QProxyStyle::subControlRect(control, option, subControl, widget);
    }

    void drawPrimitive(
        PrimitiveElement element,
        const QStyleOption* option,
        QPainter* painter,
        const QWidget* widget = nullptr) const override
    {
        switch (element) {
        case PE_PanelButtonTool:
            drawToolPanel_(option, painter);
            return;

        case PE_FrameButtonTool:
        case PE_FrameDefaultButton:
        case PE_FrameTabBarBase:
            return;

        case PE_PanelButtonCommand:
            drawCommandPanel_(option, painter);
            return;

        case PE_PanelLineEdit:
            if (drawLineEditPanel_(option, painter)) {
                return;
            }
            break;

        case PE_FrameLineEdit:
            // The panel draws its own border
            return;

        case PE_PanelItemViewItem:
            drawItemPanel_(option, painter);
            return;

        case PE_PanelItemViewRow:
            // Rows aren't banded, and selection is the item's to draw
            return;

        case PE_PanelMenu:
            drawMenuPanel_(option, painter);
            return;

        case PE_FrameMenu:
            drawMenuBorder_(option, painter);
            return;

        case PE_Frame:
            // A drop-down list's window is a QFrame, framed by this
            if (isComboPopup_(widget)) {
                drawMenuBorder_(option, painter);
                return;
            }
            break;

        case PE_FrameFocusRect:
            // The rows' wash already says which one is current
            if (qobject_cast<const QAbstractItemView*>(widget)) {
                return;
            }
            break;

        default:
            break;
        }

        QProxyStyle::drawPrimitive(element, option, painter, widget);
    }

    void drawControl(
        ControlElement element,
        const QStyleOption* option,
        QPainter* painter,
        const QWidget* widget = nullptr) const override
    {
        switch (element) {
        case CE_Splitter:
            painter->fillRect(option->rect, divider(option->palette));
            return;

        case CE_ShapedFrame:
            if (drawItemViewFrame_(option, painter, widget)) {
                return;
            }
            break;

        case CE_SizeGrip:
        case CE_MenuEmptyArea:
            // The menu's panel is already drawn, corners and all
            return;

        case CE_TabBarTabShape:
            if (auto* tab = qstyleoption_cast<const QStyleOptionTab*>(option)) {
                drawTabShape_(tab, painter);
                return;
            }
            break;

        case CE_TabBarTabLabel:
            if (auto* tab = qstyleoption_cast<const QStyleOptionTab*>(option)) {
                // The label (icon and title together) is laid out in the part
                // of the tab that's drawn, below the gap at its top, so both
                // center there. A tab that isn't current reads quieter than
                // the current one
                QStyleOptionTab copy(*tab);
                copy.rect.setTop(copy.rect.top() + STYLE_TAB_TOP_GAP);
                if (!(tab->state & State_Selected)) {
                    setAllGroups_(
                        copy.palette,
                        QPalette::WindowText,
                        wash_(tab->palette, STYLE_MUTED_TEXT_ALPHA));
                }
                QProxyStyle::drawControl(element, &copy, painter, widget);
                return;
            }
            break;

        case CE_ItemViewItem:
            if (auto* item =
                    qstyleoption_cast<const QStyleOptionViewItem*>(option)) {
                // The wash is drawn here, before Fusion clips its painting to
                // the item's cell, so in a tree it can reach back over the
                // chevron. Then Fusion draws the item as if neither selected
                // nor hovered: the wash is the selection, so the text keeps
                // its color and an icon isn't tinted
                drawItemWash_(item, painter, widget);

                QStyleOptionViewItem copy(*item);
                copy.state &= ~(State_Selected | State_MouseOver);
                QProxyStyle::drawControl(element, &copy, painter, widget);
                return;
            }
            break;

        case CE_PushButtonLabel:
            if (auto* button =
                    qstyleoption_cast<const QStyleOptionButton*>(option)) {
                // A default button's label sits on the accent
                if (isDefaultButton_(button)) {
                    QStyleOptionButton copy(*button);
                    setAllGroups_(
                        copy.palette,
                        QPalette::ButtonText,
                        button->palette.color(QPalette::HighlightedText));
                    QProxyStyle::drawControl(element, &copy, painter, widget);
                    return;
                }
            }
            break;

        case CE_MenuItem:
            if (auto* item =
                    qstyleoption_cast<const QStyleOptionMenuItem*>(option)) {
                drawMenuItem_(item, painter, widget);
                return;
            }
            break;

        case CE_MenuBarItem:
            if (auto* item =
                    qstyleoption_cast<const QStyleOptionMenuItem*>(option)) {
                painter->fillRect(
                    item->rect,
                    item->palette.brush(QPalette::Window));

                if (item->state & (State_Selected | State_Sunken)) {
                    fillRounded_(
                        painter,
                        QRectF(item->rect).adjusted(1, 2, -1, -2),
                        STYLE_ROW_RADIUS,
                        wash_(item->palette, STYLE_SELECTED_ALPHA));
                }

                QStyleOptionMenuItem copy(*item);
                copy.state &= ~(State_Selected | State_Sunken);
                QProxyStyle::drawControl(element, &copy, painter, widget);
                return;
            }
            break;

        default:
            break;
        }

        QProxyStyle::drawControl(element, option, painter, widget);
    }

    void drawComplexControl(
        ComplexControl control,
        const QStyleOptionComplex* option,
        QPainter* painter,
        const QWidget* widget = nullptr) const override
    {
        switch (control) {
        case CC_ScrollBar:
            if (auto* bar =
                    qstyleoption_cast<const QStyleOptionSlider*>(option)) {
                drawScrollBar_(bar, painter, widget);
                return;
            }
            break;

        case CC_Slider:
            if (auto* slider =
                    qstyleoption_cast<const QStyleOptionSlider*>(option)) {
                if (slider->orientation == Qt::Horizontal) {
                    drawSlider_(slider, painter, widget);
                    return;
                }
            }
            break;

        case CC_ComboBox:
            if (auto* combo =
                    qstyleoption_cast<const QStyleOptionComboBox*>(option)) {
                if (!combo->editable) {
                    drawComboBox_(combo, painter, widget);
                    return;
                }
            }
            break;

        default:
            break;
        }

        QProxyStyle::drawComplexControl(control, option, painter, widget);
    }

private:
    static constexpr auto MENU_WINDOW_FLAGS_ =
        Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint;

    // A combo box's drop-down list: the popup window Qt makes for it, as a
    // child of the combo box. Its class is private to Qt, so it's known by
    // where it sits instead. Only a combo box that can't be typed in gets a
    // menu-style list (Fusion's SH_ComboBox_Popup), whose window Qt fills with
    // the menu panel; an editable one's list is left as Fusion draws it
    [[nodiscard]] static bool isComboPopup_(const QWidget* widget)
    {
        if (!widget || widget->windowType() != Qt::Popup) {
            return false;
        }

        auto* combo = qobject_cast<const QComboBox*>(widget->parentWidget());
        return combo && !combo->isEditable();
    }

    static void setHoverTracked_(QWidget* widget, bool on)
    {
        if (qobject_cast<QTabBar*>(widget)) {
            widget->setAttribute(Qt::WA_Hover, on);
        } else if (auto* view = qobject_cast<QAbstractItemView*>(widget)) {
            view->viewport()->setAttribute(Qt::WA_Hover, on);
        }
    }

    // A palette's text color at a low alpha: the shade for hovers, presses,
    // selections, and borders. Drawn over whatever is behind it, so it's the
    // right step darker on a light background and lighter on a dark one
    [[nodiscard]] static QColor wash_(const QPalette& palette, qreal alpha)
    {
        auto color = palette.color(QPalette::WindowText);
        color.setAlphaF(alpha);
        return color;
    }

    [[nodiscard]] static QColor
    blend_(const QColor& from, const QColor& to, qreal amount)
    {
        return QColor::fromRgbF(
            from.redF() + (to.redF() - from.redF()) * amount,
            from.greenF() + (to.greenF() - from.greenF()) * amount,
            from.blueF() + (to.blueF() - from.blueF()) * amount,
            from.alphaF() + (to.alphaF() - from.alphaF()) * amount);
    }

    static void setAllGroups_(
        QPalette& palette,
        QPalette::ColorRole role,
        const QColor& color)
    {
        palette.setColor(QPalette::Active, role, color);
        palette.setColor(QPalette::Inactive, role, color);
        palette.setColor(QPalette::Disabled, role, color);
    }

    static void fillRounded_(
        QPainter* painter,
        const QRectF& rect,
        qreal radius,
        const QColor& color)
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setPen(Qt::NoPen);
        painter->setBrush(color);
        painter->drawRoundedRect(rect, radius, radius);
        painter->restore();
    }

    // A rounded rectangle with a hairline border, drawn half a pixel in so the
    // line lands on whole pixels
    static void drawBordered_(
        QPainter* painter,
        const QRect& rect,
        const QBrush& fill,
        const QColor& border)
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setPen(QPen(border, 1.0));
        painter->setBrush(fill);
        painter->drawRoundedRect(
            QRectF(rect).adjusted(0.5, 0.5, -0.5, -0.5),
            STYLE_CONTROL_RADIUS,
            STYLE_CONTROL_RADIUS);
        painter->restore();
    }

    [[nodiscard]] static bool isDefaultButton_(const QStyleOptionButton* button)
    {
        return (button->features & QStyleOptionButton::DefaultButton) &&
               (button->state & State_Enabled);
    }

    // --- Tool buttons ------------------------------------------------------

    static void drawToolPanel_(const QStyleOption* option, QPainter* painter)
    {
        if (!(option->state & State_Enabled)) {
            return;
        }

        auto pressed = bool(option->state & (State_Sunken | State_On));
        if (!pressed && !(option->state & State_MouseOver)) {
            return;
        }

        fillRounded_(
            painter,
            QRectF(option->rect),
            STYLE_TOOL_BUTTON_RADIUS,
            buttonWash(option->palette, pressed));
    }

    // --- Push buttons, combo boxes, line edits -----------------------------

    static void drawCommandPanel_(const QStyleOption* option, QPainter* painter)
    {
        auto* button = qstyleoption_cast<const QStyleOptionButton*>(option);
        auto pressed = option->state & (State_Sunken | State_On);
        auto hovered = option->state & State_MouseOver;

        if (button && isDefaultButton_(button)) {
            auto accent = option->palette.color(QPalette::Highlight);
            if (pressed) {
                accent = accent.darker(115);
            } else if (hovered) {
                accent = accent.lighter(108);
            }
            drawBordered_(painter, option->rect, accent, accent);
            return;
        }

        drawPlainPanel_(option, painter, pressed, hovered);
    }

    // The neutral raised surface shared by push buttons and combo boxes: the
    // palette's button color, a hairline border, and a wash on hover and
    // press. polish lightens a light palette's window color but not its
    // button color, so there a button is usually a shade darker than the
    // window around it
    static void drawPlainPanel_(
        const QStyleOption* option,
        QPainter* painter,
        bool pressed,
        bool hovered)
    {
        const auto& palette = option->palette;
        auto focused = (option->state & State_HasFocus) &&
                       (option->state & State_KeyboardFocusChange);

        drawBordered_(
            painter,
            option->rect,
            palette.brush(QPalette::Button),
            focused ? palette.color(QPalette::Highlight)
                    : wash_(palette, STYLE_CONTROL_BORDER_ALPHA));

        if (pressed || hovered) {
            fillRounded_(
                painter,
                QRectF(option->rect).adjusted(1, 1, -1, -1),
                STYLE_CONTROL_RADIUS - 1,
                wash_(
                    palette,
                    pressed ? STYLE_PRESSED_ALPHA : STYLE_HOVER_ALPHA));
        }
    }

    // False when there's no frame to draw (a frameless field), which Fusion
    // handles as it always has
    static bool
    drawLineEditPanel_(const QStyleOption* option, QPainter* painter)
    {
        auto* frame = qstyleoption_cast<const QStyleOptionFrame*>(option);
        if (!frame || frame->lineWidth <= 0) {
            return false;
        }

        const auto& palette = option->palette;
        auto focused = option->state & State_HasFocus;

        drawBordered_(
            painter,
            option->rect,
            palette.brush(QPalette::Base),
            focused ? palette.color(QPalette::Highlight)
                    : wash_(palette, STYLE_CONTROL_BORDER_ALPHA));
        return true;
    }

    void drawComboBox_(
        const QStyleOptionComboBox* combo,
        QPainter* painter,
        const QWidget* widget) const
    {
        auto pressed = combo->state & State_On;
        auto hovered = combo->state & State_MouseOver;
        drawPlainPanel_(combo, painter, pressed, hovered);

        // Fusion's arrow, in the arrow's own sub-rect. The label is QComboBox's
        // to draw (CE_ComboBoxLabel), after this
        auto arrow_rect =
            subControlRect(CC_ComboBox, combo, SC_ComboBoxArrow, widget);
        QStyleOption arrow(*combo);
        arrow.rect = arrow_rect.adjusted(
            STYLE_COMBO_ARROW_INSET,
            STYLE_COMBO_ARROW_INSET,
            -STYLE_COMBO_ARROW_INSET,
            -STYLE_COMBO_ARROW_INSET);
        setAllGroups_(
            arrow.palette,
            QPalette::ButtonText,
            wash_(combo->palette, STYLE_MUTED_TEXT_ALPHA));
        QProxyStyle::drawPrimitive(
            PE_IndicatorArrowDown,
            &arrow,
            painter,
            widget);
    }

    // A thin rounded track, the accent up to the handle, and a round handle.
    // The groove and handle are where Fusion puts them, so dragging and
    // clicking behave as Fusion's do
    void drawSlider_(
        const QStyleOptionSlider* slider,
        QPainter* painter,
        const QWidget* widget) const
    {
        const auto& palette = slider->palette;
        auto groove =
            subControlRect(CC_Slider, slider, SC_SliderGroove, widget);
        auto handle =
            subControlRect(CC_Slider, slider, SC_SliderHandle, widget);

        auto track_height = qreal(STYLE_SLIDER_TRACK_HEIGHT);
        auto track = QRectF(
            groove.left(),
            groove.center().y() - track_height / 2 + 0.5,
            groove.width(),
            track_height);
        auto radius = track_height / 2;

        fillRounded_(
            painter,
            track,
            radius,
            wash_(palette, STYLE_SLIDER_TRACK_ALPHA));

        auto enabled = slider->state & State_Enabled;
        auto filled = track;
        filled.setRight(handle.center().x());
        fillRounded_(
            painter,
            filled,
            radius,
            enabled ? palette.color(QPalette::Highlight)
                    : wash_(palette, STYLE_PRESSED_ALPHA));

        auto diameter = qreal(STYLE_SLIDER_HANDLE_DIAMETER);
        auto knob = QRectF(
            handle.center().x() + 0.5 - diameter / 2,
            handle.center().y() + 0.5 - diameter / 2,
            diameter,
            diameter);

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setPen(QPen(wash_(palette, STYLE_CONTROL_BORDER_ALPHA), 1.0));
        painter->setBrush(palette.brush(QPalette::Base));
        painter->drawEllipse(knob.adjusted(0.5, 0.5, -0.5, -0.5));
        painter->restore();

        if (slider->activeSubControls & SC_SliderHandle &&
            slider->state & (State_MouseOver | State_Sunken)) {
            fillRounded_(
                painter,
                knob.adjusted(1, 1, -1, -1),
                diameter / 2,
                wash_(
                    palette,
                    slider->state & State_Sunken ? STYLE_PRESSED_ALPHA
                                                 : STYLE_HOVER_ALPHA));
        }
    }

    // --- Menus -------------------------------------------------------------

    // Fusion keeps room left of every item's text for a check mark or an
    // icon, even in a menu that has neither. There, some of that room is taken
    // back (STYLE_MENU_TEXT_TRIM)
    [[nodiscard]] static bool trimsMenuText_(const QStyleOptionMenuItem* item)
    {
        return item->maxIconWidth == 0 && !item->menuHasCheckableItems;
    }

    // A selected item gets the rounded wash and is then drawn by Fusion as if
    // unselected, so its text keeps its color. Where the text is trimmed,
    // Fusion is handed a rect that starts further left, clipped to the item's
    // own, so the text moves left and nothing is drawn outside the item
    void drawMenuItem_(
        const QStyleOptionMenuItem* item,
        QPainter* painter,
        const QWidget* widget) const
    {
        if (item->menuItemType == QStyleOptionMenuItem::Separator) {
            QProxyStyle::drawControl(CE_MenuItem, item, painter, widget);
            return;
        }

        QStyleOptionMenuItem copy(*item);

        if (item->state & State_Selected) {
            fillRounded_(
                painter,
                QRectF(item->rect)
                    .adjusted(
                        STYLE_MENU_ITEM_INSET,
                        1,
                        -STYLE_MENU_ITEM_INSET,
                        -1),
                STYLE_ROW_RADIUS,
                wash_(item->palette, STYLE_SELECTED_ALPHA));
            copy.state &= ~State_Selected;
        }

        if (!trimsMenuText_(item)) {
            QProxyStyle::drawControl(CE_MenuItem, &copy, painter, widget);
            return;
        }

        painter->save();
        painter->setClipRect(item->rect);
        copy.rect.setLeft(copy.rect.left() - STYLE_MENU_TEXT_TRIM);
        QProxyStyle::drawControl(CE_MenuItem, &copy, painter, widget);
        painter->restore();
    }

    // The menu's panel inside its window, which is see-through (polish): the
    // window's margin (PM_MenuPanelWidth) holds a soft shadow, built from
    // rounded rectangles stepping outward and fading
    [[nodiscard]] static QRectF menuPanelRect_(const QStyleOption* option)
    {
        auto margin = qreal(STYLE_MENU_SHADOW);
        return QRectF(option->rect).adjusted(margin, margin, -margin, -margin);
    }

    static void drawMenuPanel_(const QStyleOption* option, QPainter* painter)
    {
        auto panel = menuPanelRect_(option);
        auto radius = qreal(STYLE_MENU_RADIUS);

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setPen(Qt::NoPen);

        for (auto step = STYLE_MENU_SHADOW; step > 0; --step) {
            auto alpha = STYLE_MENU_SHADOW_ALPHA *
                         (STYLE_MENU_SHADOW - step + 1) / STYLE_MENU_SHADOW;
            painter->setBrush(QColor::fromRgbF(0, 0, 0, alpha));
            painter->drawRoundedRect(
                panel.adjusted(-step, -step + 1, step, step + 1),
                radius + step,
                radius + step);
        }

        painter->setBrush(option->palette.brush(QPalette::Base));
        painter->drawRoundedRect(panel, radius, radius);
        painter->restore();
    }

    static void drawMenuBorder_(const QStyleOption* option, QPainter* painter)
    {
        auto panel = menuPanelRect_(option).adjusted(0.5, 0.5, -0.5, -0.5);
        auto radius = qreal(STYLE_MENU_RADIUS);

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setPen(
            QPen(wash_(option->palette, STYLE_CONTROL_BORDER_ALPHA), 1.0));
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(panel, radius, radius);
        painter->restore();
    }

    // --- Tabs --------------------------------------------------------------

    // Every tab is drawn below a gap at its top, so the bar shows a strip of
    // its own color above the tabs and the current tab's rounded corners have
    // something to be rounded against. The current tab is filled with the
    // page's color (Base) right down to the bar's bottom edge, where the page
    // begins
    static void drawTabShape_(const QStyleOptionTab* tab, QPainter* painter)
    {
        auto rect = QRectF(tab->rect).adjusted(0, STYLE_TAB_TOP_GAP, 0, 0);
        const auto& palette = tab->palette;

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setPen(Qt::NoPen);

        if (tab->state & State_Selected) {
            auto radius = qreal(STYLE_TAB_RADIUS);
            QPainterPath path{};
            path.moveTo(rect.left(), rect.bottom());
            path.lineTo(rect.left(), rect.top() + radius);
            path.quadTo(
                rect.left(),
                rect.top(),
                rect.left() + radius,
                rect.top());
            path.lineTo(rect.right() - radius, rect.top());
            path.quadTo(
                rect.right(),
                rect.top(),
                rect.right(),
                rect.top() + radius);
            path.lineTo(rect.right(), rect.bottom());
            path.closeSubpath();

            painter->setBrush(palette.brush(QPalette::Base));
            painter->drawPath(path);
            painter->restore();
            return;
        }

        if (tab->state & State_MouseOver) {
            painter->setBrush(wash_(palette, STYLE_HOVER_ALPHA));
            auto inset = STYLE_TAB_HOVER_INSET;
            painter->drawRoundedRect(
                rect.adjusted(inset, inset, -inset, -inset),
                STYLE_TOOL_BUTTON_RADIUS,
                STYLE_TOOL_BUTTON_RADIUS);
        } else if (tab->selectedPosition != QStyleOptionTab::NextIsSelected) {
            // A short divider at the right edge, unless the current tab is
            // next: its own shape separates them
            auto x = rect.right() - 0.5;
            auto margin = STYLE_TAB_DIVIDER_MARGIN;
            painter->setPen(QPen(divider(palette), 1.0));
            painter->drawLine(
                QPointF(x, rect.top() + margin),
                QPointF(x, rect.bottom() - margin));
        }

        painter->restore();
    }

    // --- Item views --------------------------------------------------------

    // An item's own background, if its model gives it one, and the wash for
    // anything that draws an item panel directly. Items drawn the usual way
    // (CE_ItemViewItem) arrive here unselected and unhovered, their wash
    // already drawn by drawItemWash_
    static void drawItemPanel_(const QStyleOption* option, QPainter* painter)
    {
        auto* item = qstyleoption_cast<const QStyleOptionViewItem*>(option);
        if (item && item->backgroundBrush.style() != Qt::NoBrush) {
            painter->fillRect(option->rect, item->backgroundBrush);
        }

        fillRowWash_(option, QRectF(option->rect), painter);
    }

    // A tree row's cell starts after its indentation, and the chevron sits in
    // the last indentation step before the cell. The row's wash reaches back
    // over that step, so a folder's highlight takes in its chevron
    [[nodiscard]] static QRectF
    itemRowRect_(const QRect& itemRect, const QWidget* widget)
    {
        auto rect = QRectF(itemRect);

        if (auto* tree = qobject_cast<const QTreeView*>(widget)) {
            rect.setLeft(qMax(qreal(0), rect.left() - tree->indentation()));
        }

        return rect;
    }

    static void drawItemWash_(
        const QStyleOptionViewItem* item,
        QPainter* painter,
        const QWidget* widget)
    {
        auto rect = itemRowRect_(item->rect, widget);
        auto* tree = qobject_cast<const QTreeView*>(widget);

        fillRowWash_(item, rect, painter);

        // A list's selected row is marked at its left end in the accent, as
        // the rows of a navigation list are. A tree's selection is the wash
        // alone, since its rows are files rather than places to go
        if (!tree && (item->state & State_Selected)) {
            auto height = qMin(
                qreal(STYLE_LIST_INDICATOR_HEIGHT),
                rect.height() - 2 * STYLE_ROW_INSET);
            auto indicator = QRectF(
                rect.left() + STYLE_ROW_INSET + STYLE_LIST_INDICATOR_INSET,
                rect.center().y() - height / 2,
                STYLE_LIST_INDICATOR_WIDTH,
                height);
            fillRounded_(
                painter,
                indicator,
                STYLE_LIST_INDICATOR_WIDTH / 2.0,
                item->palette.color(QPalette::Highlight));
        }
    }

    // A framed item view (Go to File's list, a dialog's list) is a rounded
    // panel in its viewport's color, with a hairline border. The viewport
    // paints no background of its own (polish), so the panel shows through it
    // and its rounded corners aren't covered by the viewport's square ones.
    // False for any other frame, which is Fusion's
    static bool drawItemViewFrame_(
        const QStyleOption* option,
        QPainter* painter,
        const QWidget* widget)
    {
        auto* view = qobject_cast<const QAbstractItemView*>(widget);
        auto* frame = qstyleoption_cast<const QStyleOptionFrame*>(option);
        if (!view || !frame || frame->frameShape != QFrame::StyledPanel) {
            return false;
        }

        auto* viewport = view->viewport();
        drawBordered_(
            painter,
            option->rect,
            viewport->palette().brush(viewport->backgroundRole()),
            wash_(option->palette, STYLE_CONTROL_BORDER_ALPHA));
        return true;
    }

    // A disabled row (a settings group title) gets no hover wash: an item
    // view marks the row under the mouse as hovered whether or not it's
    // enabled
    static void fillRowWash_(
        const QStyleOption* option,
        const QRectF& rect,
        QPainter* painter)
    {
        auto selected = option->state & State_Selected;
        auto hovered = (option->state & State_MouseOver) &&
                       (option->state & State_Enabled);
        if (!selected && !hovered) {
            return;
        }

        auto alpha = selected && hovered ? STYLE_SELECTED_HOVER_ALPHA
                     : selected          ? STYLE_SELECTED_ALPHA
                                         : STYLE_HOVER_ALPHA;

        fillRounded_(
            painter,
            washRect_(rect),
            STYLE_ROW_RADIUS,
            wash_(option->palette, alpha));
    }

    // A row's wash is inset from the view's sides, and a pixel from the rows
    // above and below, so neighboring washes don't touch
    [[nodiscard]] static QRectF washRect_(const QRectF& rowRect)
    {
        return rowRect.adjusted(STYLE_ROW_INSET, 1, -STYLE_ROW_INSET, -1);
    }

    // --- Scroll bars -------------------------------------------------------

    // The handle's place along the bar, computed here rather than by Fusion:
    // with no arrow buttons the whole bar is the groove. QCommonStyle's hit
    // testing asks for these rects through the proxy, so clicks and drags
    // agree with what's drawn
    [[nodiscard]] static QRect
    scrollBarRect_(const QStyleOptionSlider* bar, SubControl subControl)
    {
        auto rect = bar->rect;
        auto horizontal = bar->orientation == Qt::Horizontal;
        auto length = horizontal ? rect.width() : rect.height();
        auto range = qint64(bar->maximum) - bar->minimum;

        auto handle = length;
        if (range > 0) {
            handle =
                int(qint64(length) * bar->pageStep / (range + bar->pageStep));
            handle = qBound(
                qMin(STYLE_SCROLL_BAR_HANDLE_MIN, length),
                handle,
                length);
        }

        auto position = QStyle::sliderPositionFromValue(
            bar->minimum,
            bar->maximum,
            bar->sliderPosition,
            length - handle,
            bar->upsideDown);

        auto along = [&](int start, int size) {
            return horizontal
                       ? QRect(rect.x() + start, rect.y(), size, rect.height())
                       : QRect(rect.x(), rect.y() + start, rect.width(), size);
        };

        switch (subControl) {
        case SC_ScrollBarGroove:
            return rect;
        case SC_ScrollBarSlider:
            return along(position, handle);
        case SC_ScrollBarSubPage:
            return along(0, position);
        case SC_ScrollBarAddPage:
            return along(position + handle, length - position - handle);
        default:
            return {};
        }
    }

    // The bar's background is the scrolled content's, so a scroll bar beside
    // the editor isn't a strip of window color down the page's edge. A scroll
    // area keeps its bars in a container widget of its own, so the area is
    // the bar's grandparent
    [[nodiscard]] static QBrush grooveBrush_(const QWidget* bar)
    {
        for (auto* parent = bar ? bar->parentWidget() : nullptr; parent;
             parent = parent->parentWidget()) {
            if (auto* area = qobject_cast<const QAbstractScrollArea*>(parent)) {
                auto* viewport = area->viewport();
                return viewport->palette().brush(viewport->backgroundRole());
            }
        }

        return {};
    }

    static void drawScrollBar_(
        const QStyleOptionSlider* bar,
        QPainter* painter,
        const QWidget* widget)
    {
        if (auto brush = grooveBrush_(widget); brush.style() != Qt::NoBrush) {
            painter->fillRect(bar->rect, brush);
        }

        if (bar->minimum == bar->maximum) {
            return;
        }

        auto active = bar->activeSubControls & SC_ScrollBarSlider;
        auto alpha = (active && (bar->state & State_Sunken))
                         ? STYLE_SCROLL_BAR_PRESSED_ALPHA
                     : (active && (bar->state & State_MouseOver))
                         ? STYLE_SCROLL_BAR_HOVER_ALPHA
                         : STYLE_SCROLL_BAR_ALPHA;

        auto handle = QRectF(scrollBarRect_(bar, SC_ScrollBarSlider));
        auto inset = STYLE_SCROLL_BAR_HANDLE_INSET;
        handle.adjust(inset, inset, -inset, -inset);

        auto radius = qMin(handle.width(), handle.height()) / 2;
        fillRounded_(painter, handle, radius, wash_(bar->palette, alpha));
    }
};

} // namespace Fernanda::Ui
