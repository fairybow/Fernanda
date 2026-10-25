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

#include <optional>

#include <QByteArray>
#include <QByteArrayView>
#include <QList>
#include <QString>

// One file's changes, read from what `git diff` prints in its unified format.
// Free functions with no process, so the reading is testable on its own;
// GitRepository runs git and asks.
//
// Only the hunks are read. The lines before the first one (diff --git, index,
// the --- and +++ names, mode changes) say what GitRepository already knows,
// apart from whether the file is binary, which git says there instead of
// giving hunks. Inside a hunk every line starts with a space, a plus, a minus,
// or a backslash (git's "\ No newline at end of file"), so any other line ends
// it.
//
// A line's text is UTF-8, without its line break. A carriage return before
// the break, from a file with Windows line endings, is dropped from every
// line, since the diff view shows lines, not their endings
namespace Fernanda::GitDiff {

struct Line
{
    enum class Kind
    {
        Context,
        Added,
        Removed
    };

    Kind kind = Kind::Context;
    QString text{};

    // Its number in the old file and in the new one, counted from 1. 0 where
    // the line isn't in that file: an added line has no old number, and a
    // removed one no new number
    int oldNumber = 0;
    int newNumber = 0;
};

struct Hunk
{
    // Where the hunk starts in each file
    int oldStart = 0;
    int newStart = 0;

    // The @@ line as git wrote it, which may end with the nearest line above
    // that git took for a heading
    QString header{};

    QList<Line> lines{};
};

struct Diff
{
    // Git said the file is binary, and gave no hunks
    bool binary = false;

    // Empty for a binary file, and for a change git shows no lines for (an
    // empty file added, a mode changed)
    QList<Hunk> hunks{};
};

namespace Internal {

// "12" or "12,4" (a start, then a count this doesn't need)
[[nodiscard]] inline std::optional<int> startOf_(QByteArrayView range)
{
    auto comma = range.indexOf(',');
    auto start = comma < 0 ? range : range.first(comma);

    auto ok = false;
    auto value = start.toInt(&ok);

    return ok ? std::optional<int>(value) : std::nullopt;
}

// "@@ -12,4 +12,5 @@ heading". Nothing when the line isn't one
[[nodiscard]] inline std::optional<Hunk> hunkOf_(QByteArrayView line)
{
    if (!line.startsWith("@@ -")) {
        return std::nullopt;
    }

    auto plus = line.indexOf(" +", 4);
    auto end = line.indexOf(" @@", 4);

    if (plus < 0 || end < 0 || plus > end) {
        return std::nullopt;
    }

    auto old_start = startOf_(line.sliced(4, plus - 4));
    auto new_start = startOf_(line.sliced(plus + 2, end - plus - 2));

    if (!old_start || !new_start) {
        return std::nullopt;
    }

    Hunk hunk{};
    hunk.oldStart = *old_start;
    hunk.newStart = *new_start;
    hunk.header = QString::fromUtf8(line);
    return hunk;
}

[[nodiscard]] inline QString textOf_(QByteArrayView line)
{
    return QString::fromUtf8(line.sliced(1));
}

} // namespace Internal

[[nodiscard]] inline Diff parse(const QByteArray& output)
{
    Diff diff{};

    // The hunk being read, and the next line numbers in it
    Hunk* hunk = nullptr;
    auto old_number = 0;
    auto new_number = 0;

    for (QByteArrayView line : output.split('\n')) {
        if (line.endsWith('\r')) {
            line.chop(1);
        }

        if (hunk) {
            if (line.startsWith(' ')) {
                hunk->lines << Line{ Line::Kind::Context,
                                     Internal::textOf_(line),
                                     old_number++,
                                     new_number++ };
                continue;
            }

            if (line.startsWith('+')) {
                hunk->lines << Line{ Line::Kind::Added,
                                     Internal::textOf_(line),
                                     0,
                                     new_number++ };
                continue;
            }

            if (line.startsWith('-')) {
                hunk->lines << Line{ Line::Kind::Removed,
                                     Internal::textOf_(line),
                                     old_number++,
                                     0 };
                continue;
            }

            if (line.startsWith('\\')) {
                continue;
            }

            hunk = nullptr;
        }

        if (auto next = Internal::hunkOf_(line)) {
            diff.hunks << *next;
            hunk = &diff.hunks.last();
            old_number = hunk->oldStart;
            new_number = hunk->newStart;
            continue;
        }

        if (diff.hunks.isEmpty() && (line.startsWith("Binary files ") ||
                                     line.startsWith("GIT binary patch"))) {
            diff.binary = true;
        }
    }

    return diff;
}

} // namespace Fernanda::GitDiff
