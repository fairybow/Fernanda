/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
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

#include <optional>

#include <QByteArray>
#include <QByteArrayView>
#include <QList>
#include <QString>
#include <QtTypes>

#include <Coco/Path.h>

// A repository's status, read from what `git status --porcelain=v2 --branch
// -z` prints. Free functions with no process, so the reading is testable on
// its own; GitRepository runs git and asks.
//
// Porcelain v2 is the format Git keeps stable for programs. With -z, each
// record ends in a NUL and no path is quoted, so a path may hold spaces or any
// other character. A path is relative to the repository's folder.
//
// Each file gets one change, as GitHub Desktop shows it: the index and the
// working tree are not told apart, since Fernanda stages for itself when it
// commits. A folder holding a repository of its own, which git lists as one
// untracked entry, is left out (parse)
namespace Fernanda::GitStatus {

enum class Change
{
    Added,
    Modified,
    Deleted,
    Renamed,
    Untracked,
    Conflicted
};

struct Entry
{
    Change change = Change::Modified;

    // Relative to the repository's folder
    Coco::Path path{};

    // Where a renamed file was. Empty for any other change
    Coco::Path originalPath{};
};

struct Branch
{
    // Empty when detached
    QString name{};
    bool detached = false;

    // No commit yet: a new repository, or a new orphan branch
    bool unborn = false;

    // The branch it tracks, such as origin/main. Empty when none
    QString upstream{};

    // Whether the upstream exists, so ahead and behind are known. False with
    // no upstream, and when the upstream is named but gone: deleted on the
    // remote and pruned by a fetch. Git prints how far ahead and behind the
    // branch is only when it has an upstream to count against
    bool upstreamFound = false;

    // Commits this branch has that its upstream lacks, and the reverse.
    // Both zero unless upstreamFound
    int ahead = 0;
    int behind = 0;
};

struct Status
{
    Branch branch{};
    QList<Entry> entries{};
};

namespace Internal {

// The path at the end of a record, after the given number of space-separated
// fields. Nothing when the record is too short
[[nodiscard]] inline std::optional<QByteArrayView>
pathAfter_(QByteArrayView record, int fieldsBefore)
{
    qsizetype start = 0;

    for (auto i = 0; i < fieldsBefore; ++i) {
        auto space = record.indexOf(' ', start);

        if (space < 0) {
            return std::nullopt;
        }

        start = space + 1;
    }

    if (start >= record.size()) {
        return std::nullopt;
    }

    return record.sliced(start);
}

[[nodiscard]] inline Coco::Path pathOf_(QByteArrayView bytes)
{
    return QString::fromUtf8(bytes);
}

// One change for an ordinary record's index (x) and working tree (y)
// letters. A file added to the index is added, even if it's since been
// deleted from the working tree: it isn't in the last commit, so it's new
// either way. So is a file marked to be added (git add -N), whose letter is
// on the working tree's side. Otherwise a deletion on either side is a
// deletion, and anything else (modified, type changed) is modified
[[nodiscard]] inline Change changeOf_(char x, char y)
{
    if (x == 'A' || y == 'A') {
        return Change::Added;
    }

    if (x == 'D' || y == 'D') {
        return Change::Deleted;
    }

    return Change::Modified;
}

// "+3 -1" after "# branch.ab "
inline void readAheadBehind_(QByteArrayView value, Branch& branch)
{
    auto space = value.indexOf(' ');

    if (space < 2 || value.front() != '+') {
        return;
    }

    auto behind = value.sliced(space + 1);

    if (behind.size() < 2 || behind.front() != '-') {
        return;
    }

    auto ok_ahead = false;
    auto ok_behind = false;
    auto ahead_count = value.sliced(1, space - 1).toInt(&ok_ahead);
    auto behind_count = behind.sliced(1).toInt(&ok_behind);

    if (ok_ahead && ok_behind) {
        branch.upstreamFound = true;
        branch.ahead = ahead_count;
        branch.behind = behind_count;
    }
}

inline void readHeader_(QByteArrayView record, Branch& branch)
{
    auto read =
        [record](QByteArrayView prefix) -> std::optional<QByteArrayView> {
        if (!record.startsWith(prefix)) {
            return std::nullopt;
        }

        return record.sliced(prefix.size());
    };

    if (auto value = read("# branch.oid ")) {
        branch.unborn = *value == "(initial)";
    } else if (auto value = read("# branch.head ")) {
        branch.detached = *value == "(detached)";
        branch.name = branch.detached ? QString{} : QString::fromUtf8(*value);
    } else if (auto value = read("# branch.upstream ")) {
        branch.upstream = QString::fromUtf8(*value);
    } else if (auto value = read("# branch.ab ")) {
        readAheadBehind_(*value, branch);
    }
}

} // namespace Internal

// A record that doesn't parse is skipped, so one odd line can't hide the rest
[[nodiscard]] inline Status parse(const QByteArray& output)
{
    Status status{};

    auto records = output.split('\0');
    qsizetype i = 0;

    while (i < records.size()) {
        QByteArrayView record = records[i++];

        if (record.isEmpty()) {
            continue;
        }

        if (record.front() == '#') {
            Internal::readHeader_(record, status.branch);
            continue;
        }

        // A path ending in a slash is a folder holding a repository of its
        // own, which git lists as one entry without looking inside. It's
        // skipped: committing it would record only a pointer to the other
        // repository's commit, and discarding it would trash that repository
        // whole, its history included
        if (record.startsWith("? ")) {
            if (!record.endsWith('/')) {
                status.entries << Entry{ Change::Untracked,
                                         Internal::pathOf_(record.sliced(2)) };
            }

            continue;
        }

        // Every other kind of record has its two letters at 2 and 3
        if (record.size() < 4) {
            continue;
        }

        auto x = record[2];
        auto y = record[3];

        // 1 XY sub mH mI mW hH hI path
        if (record.front() == '1') {
            if (auto path = Internal::pathAfter_(record, 8)) {
                status.entries << Entry{ Internal::changeOf_(x, y),
                                         Internal::pathOf_(*path) };
            }

            continue;
        }

        // 2 XY sub mH mI mW hH hI Xscore path, then the original path as
        // the next record. A copy is a new file at its path
        if (record.front() == '2') {
            auto path = Internal::pathAfter_(record, 9);

            if (i >= records.size()) {
                break;
            }

            QByteArrayView original = records[i++];

            if (!path || original.isEmpty()) {
                continue;
            }

            if (x == 'R' || y == 'R') {
                status.entries << Entry{ Change::Renamed,
                                         Internal::pathOf_(*path),
                                         Internal::pathOf_(original) };
            } else {
                status.entries
                    << Entry{ Change::Added, Internal::pathOf_(*path) };
            }

            continue;
        }

        // u XY sub m1 m2 m3 mW h1 h2 h3 path
        if (record.front() == 'u') {
            if (auto path = Internal::pathAfter_(record, 10)) {
                status.entries
                    << Entry{ Change::Conflicted, Internal::pathOf_(*path) };
            }

            continue;
        }
    }

    return status;
}

} // namespace Fernanda::GitStatus
