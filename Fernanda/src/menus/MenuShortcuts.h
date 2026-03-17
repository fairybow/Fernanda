/*
 * Fernanda is a plain text editor for fiction writing
 * Copyright (C) 2025-2026, fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <QKeyCombination>

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
