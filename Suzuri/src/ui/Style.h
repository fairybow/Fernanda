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

#include <QAbstractItemView>
#include <QAbstractScrollArea>
#include <QBrush>
#include <QColor>
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

namespace Suzuri::Ui {

using namespace Qt::StringLiterals;

// Suzuri's look: Fusion, with Suzuri's own refinements on top. App installs
// one for the whole application before any window exists.
//
// Fusion rather than each platform's own style, for two reasons. Suzuri looks
// the same on every OS. And Fusion draws every standard control from the
// palette alone, which is what follows the OS's light or dark mode (on Windows
// 10 too, where the native style has no dark mode). The platform styles draw
// parts of themselves in colors of their own.
//
// The proxy is where Suzuri changes how Fusion draws: flatter, rounder, and
// quieter. Every color still comes from the palette. Where Suzuri wants a
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
//   and icon colors. In a tree it reaches back over the row's chevron. No
//   dotted focus rectangle
// - Push buttons, combo boxes, and line edits are rounded and flat, with a
//   hairline border. A default button takes the accent. A focused field's
//   border does too
// - Sliders are a thin rounded track, filled with the accent up to a round
//   handle
// - Menu items and menu bar items are highlighted with the rounded wash
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

    using QProxyStyle::polish;
    using QProxyStyle::unpolish;

    // Fusion asks for hover events on buttons, combo boxes, sliders, and scroll
    // bars, but not on tab bars or item views, whose hover washes need them
    void polish(QWidget* widget) override
    {
        QProxyStyle::polish(widget);
        setHoverTracked_(widget, true);
    }

    void unpolish(QWidget* widget) override
    {
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
            // inside the wash, which is inset STYLE_ROW_INSET
            if (qobject_cast<const QAbstractItemView*>(widget)) {
                return STYLE_ROW_TEXT_MARGIN;
            }
            break;
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

        // Rows with room around their text, for the wash to sit in
        if (type == CT_ItemViewItem) {
            result.rheight() += 2 * STYLE_ROW_V_PADDING;
        }

        return result;
    }

    QRect subElementRect(
        SubElement element,
        const QStyleOption* option,
        const QWidget* widget = nullptr) const override
    {
        auto rect = QProxyStyle::subElementRect(element, option, widget);

        // A tab's contents are centered in the part of the tab that's drawn,
        // below the gap at its top (see drawTabShape_)
        switch (element) {
        case SE_TabBarTabText:
        case SE_TabBarTabLeftButton:
        case SE_TabBarTabRightButton:
            return rect.translated(0, STYLE_TAB_TOP_GAP / 2);
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
            painter->fillRect(option->rect, divider_(option->palette));
            return;

        case CE_SizeGrip:
            return;

        case CE_TabBarTabShape:
            if (auto* tab = qstyleoption_cast<const QStyleOptionTab*>(option)) {
                drawTabShape_(tab, painter);
                return;
            }
            break;

        case CE_TabBarTabLabel:
            if (auto* tab = qstyleoption_cast<const QStyleOptionTab*>(option)) {
                // A tab that isn't current reads quieter than the current one
                QStyleOptionTab copy(*tab);
                if (!(tab->state & State_Selected)) {
                    setAllGroups_(
                        copy.palette,
                        QPalette::WindowText,
                        wash_(tab->palette, STYLE_TAB_MUTED_TEXT_ALPHA));
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
                if (item->state & State_Selected &&
                    item->menuItemType != QStyleOptionMenuItem::Separator) {
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

                    QStyleOptionMenuItem copy(*item);
                    copy.state &= ~State_Selected;
                    QProxyStyle::drawControl(element, &copy, painter, widget);
                    return;
                }
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

    [[nodiscard]] static QColor divider_(const QPalette& palette)
    {
        return wash_(palette, STYLE_DIVIDER_ALPHA);
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

        qreal alpha = 0;

        if (option->state & (State_Sunken | State_On)) {
            alpha = STYLE_PRESSED_ALPHA;
        } else if (option->state & State_MouseOver) {
            alpha = STYLE_HOVER_ALPHA;
        } else {
            return;
        }

        fillRounded_(
            painter,
            QRectF(option->rect),
            STYLE_TOOL_BUTTON_RADIUS,
            wash_(option->palette, alpha));
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
    // window's own color, a hairline border, and a wash on hover and press
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
            wash_(combo->palette, STYLE_TAB_MUTED_TEXT_ALPHA));
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
            painter->setPen(QPen(divider_(palette), 1.0));
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
    // the last indentation step before the cell. The wash reaches back over
    // that step, so a folder's highlight takes in its chevron
    static void drawItemWash_(
        const QStyleOptionViewItem* item,
        QPainter* painter,
        const QWidget* widget)
    {
        auto rect = QRectF(item->rect);
        if (auto* tree = qobject_cast<const QTreeView*>(widget)) {
            rect.setLeft(qMax(qreal(0), rect.left() - tree->indentation()));
        }

        fillRowWash_(item, rect, painter);
    }

    static void fillRowWash_(
        const QStyleOption* option,
        const QRectF& rect,
        QPainter* painter)
    {
        auto selected = option->state & State_Selected;
        auto hovered = option->state & State_MouseOver;
        if (!selected && !hovered) {
            return;
        }

        auto alpha = selected && hovered ? STYLE_SELECTED_HOVER_ALPHA
                     : selected          ? STYLE_SELECTED_ALPHA
                                         : STYLE_HOVER_ALPHA;

        fillRounded_(
            painter,
            rect.adjusted(STYLE_ROW_INSET, 1, -STYLE_ROW_INSET, -1),
            STYLE_ROW_RADIUS,
            wash_(option->palette, alpha));
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

} // namespace Suzuri::Ui
