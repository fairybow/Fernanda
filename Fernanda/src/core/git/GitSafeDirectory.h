/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
 *
 * <https://fernanda.fairybow.com>
 * <https://github.com/fairybow/Fernanda>
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
#include <QStringList>

// What Git says when it refuses a repository it doesn't trust: one owned by
// another user on the computer, as a folder on a drive formatted exFAT or FAT,
// a network share, or WSL often is. Git won't run in it until the folder is
// added to safe.directory in the user's own Git config, since a repository's
// config and hooks can run programs. A free function with no process, so it's
// testable on its own; GitRepository runs git and asks.
//
// Git's message is translated, but its last line is the command to run, and
// that names safe.directory and the folder exactly as Git wants it added:
//
//   git config --global --add safe.directory /path/to/folder
//
// Newer Git puts the folder in single quotes, with a quote inside it written
// '\'' as a shell would read it
namespace Fernanda::GitSafeDirectory {

using namespace Qt::StringLiterals;

// The folder to add to safe.directory, when error is Git's refusal to trust a
// repository. Empty for any other error
[[nodiscard]] inline QString folderOf(const QString& error)
{
    static const auto key = u"safe.directory "_s;

    for (const auto& line : error.split(u'\n')) {
        auto at = line.indexOf(key);

        if (at < 0 || !line.contains(u"--add"_s)) {
            continue;
        }

        auto folder = line.mid(at + key.size()).trimmed();

        if (folder.size() >= 2 && folder.startsWith(u'\'') &&
            folder.endsWith(u'\'')) {
            folder = folder.mid(1, folder.size() - 2);
            folder.replace(u"'\\''"_s, u"'"_s);
        }

        return folder;
    }

    return {};
}

} // namespace Fernanda::GitSafeDirectory
