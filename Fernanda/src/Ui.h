/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QColor>

#define DECLARE_UI_BUTTON_COLOR_ACCESSORS(Member)                              \
    QColor backgroundColor() const { return Member.background; }               \
    void setBackgroundColor(const QColor& color)                               \
    {                                                                          \
        if (Member.background == color) return;                                \
        Member.background = color;                                             \
        update();                                                              \
    }                                                                          \
    QColor hoverColor() const { return Member.hover; }                         \
    void setHoverColor(const QColor& color)                                    \
    {                                                                          \
        if (Member.hover == color) return;                                     \
        Member.hover = color;                                                  \
        update();                                                              \
    }                                                                          \
    QColor pressedColor() const { return Member.pressed; }                     \
    void setPressedColor(const QColor& color)                                  \
    {                                                                          \
        if (Member.pressed == color) return;                                   \
        Member.pressed = color;                                                \
        update();                                                              \
    }

namespace Fernanda::Ui {

enum class Icon
{
    ChevronDown,
    ChevronLeft,
    ChevronRight,
    ChevronUp,
    Dot,
    Plus,
    X
};

struct ButtonColors
{
    QColor background{ Qt::transparent };
    QColor hover{ Qt::transparent };
    QColor pressed{ Qt::transparent };
};

// TODO: Pass button?
[[nodiscard]] inline QColor
resolveButtonColor(const ButtonColors& colors, bool pressed, bool hovered)
{
    if (pressed && colors.pressed.isValid()) return colors.pressed;
    if (hovered && colors.hover.isValid()) return colors.hover;
    return colors.background;
}

// TODO: Generate blends based on current background for highlighting buttons
// and tabs (or other widgets) on hover/press/etc (will use Coco::Fx)

} // namespace Fernanda::Ui
