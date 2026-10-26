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

#include <QChar>
#include <QKeySequence>
#include <QString>

namespace Fernanda::Ui::MenuText {

// A menu item's text with its key after a tab, which a menu shows right-
// aligned as it would a bound shortcut's. Display only: it binds nothing, so
// an item can name a key that the window's actions or a widget's own key
// handling already answer
[[nodiscard]] inline QString
withKey(const QString& text, QKeySequence::StandardKey key)
{
    return text + QChar(QChar::Tabulation) +
           QKeySequence(key).toString(QKeySequence::NativeText);
}

} // namespace Fernanda::Ui::MenuText
