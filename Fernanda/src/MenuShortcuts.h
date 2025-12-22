/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QKeyCombination>
#include <Qt>

namespace Fernanda::MenuShortcuts {

// TODO: Any remaining key sequences
// TODO: Use platform independent key sequences where applicable
// See: https://doc.qt.io/qt-6/qkeysequence.html

constexpr auto NEW_TAB = Qt::CTRL | Qt::Key_N;
constexpr auto NEW_WINDOW = Qt::CTRL | Qt::SHIFT | Qt::Key_N;
constexpr auto OPEN_FILE = Qt::CTRL | Qt::Key_O;
constexpr auto SAVE = Qt::CTRL | Qt::Key_S;
constexpr auto SAVE_AS = Qt::CTRL | Qt::SHIFT | Qt::Key_S;
constexpr auto SAVE_ALL = Qt::CTRL | Qt::ALT | Qt::Key_S;
constexpr auto CLOSE_TAB = Qt::CTRL | Qt::Key_W;
constexpr auto CLOSE_WINDOW = Qt::CTRL | Qt::SHIFT | Qt::Key_W;
constexpr auto QUIT = Qt::CTRL | Qt::Key_Q;

constexpr auto UNDO = Qt::CTRL | Qt::Key_Z;
constexpr auto REDO = Qt::CTRL | Qt::Key_Y;
constexpr auto CUT = Qt::CTRL | Qt::Key_X;
constexpr auto COPY = Qt::CTRL | Qt::Key_C;
constexpr auto PASTE = Qt::CTRL | Qt::Key_V;
constexpr auto DEL = Qt::Key_Delete;
constexpr auto SELECT_ALL = Qt::CTRL | Qt::Key_A;

} // namespace Fernanda::MenuShortcuts
