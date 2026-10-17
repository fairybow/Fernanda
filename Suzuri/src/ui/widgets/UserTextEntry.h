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
#include <QCoreApplication>
#include <QString>
#include <QStringList>

namespace Suzuri::Ui::UserTextEntry {

using namespace Qt::StringLiterals;

// Why a file / folder / vault name might be rejected. Advisory: the create and
// rename dialogs disable their accept button and show describe(problem), but
// the disk operation (mkdir / Coco::rename) stays authoritative. Shared by
// vault creation (NewVaultDialog) and file + folder rename
enum class Problem
{
    None,
    Empty,
    ForbiddenChar,     // a control char, or one of  \ / : * ? " < > |
    ReservedName,      // CON / PRN / AUX / NUL / COM1-9 / LPT1-9 (Windows)
    LeadingDot,        // dot-entries are hidden from the tree (Suzuri rule)
    TrailingDotOrSpace // Windows silently strips these from the end
};

// The Windows-forbidden character set is the strict superset — Linux forbids
// only '/' (and NUL), macOS only '/' and ':' (both already here) — so enforcing
// it everywhere keeps names portable to any sync target, whatever OS opens the
// vault next.
//
// Not handled (docs/Future.md, "Files and file trees"): this is per-name
// character filtering only. It does NOT handle case-insensitivity collisions
// (Linux keeps "Note" and "note" apart; Windows/macOS fold them), Unicode
// normalization (macOS stores NFD, others NFC), or the 255-byte length ceiling
// — those are collision / normalization concerns, not a character filter
[[nodiscard]] inline bool isForbiddenChar_(QChar ch)
{
    if (ch.unicode() < 0x20) { // control characters
        return true;
    }

    switch (ch.unicode()) {
    case u'<':
    case u'>':
    case u':':
    case u'"':
    case u'/':
    case u'\\':
    case u'|':
    case u'?':
    case u'*':
        return true;
    default:
        return false;
    }
}

// Windows reserves these device names with OR without an extension, so the base
// before the first dot is what matters: "CON", "con.txt", "COM1.log" are all
// reserved. Case-insensitive
[[nodiscard]] inline bool isReservedName_(const QString& name)
{
    auto dot = name.indexOf(u'.');
    auto base = (dot < 0) ? name : name.left(dot);

    static const QStringList reserved = [] {
        QStringList list{ u"CON"_s, u"PRN"_s, u"AUX"_s, u"NUL"_s };
        for (auto i = 1; i <= 9; ++i) {
            list << u"COM"_s + QString::number(i)
                 << u"LPT"_s + QString::number(i);
        }
        return list;
    }();

    return reserved.contains(base, Qt::CaseInsensitive);
}

[[nodiscard]] inline Problem problemOf(const QString& name)
{
    if (name.isEmpty()) {
        return Problem::Empty;
    }

    for (auto ch : name) {
        if (isForbiddenChar_(ch)) {
            return Problem::ForbiddenChar;
        }
    }

    // Leading dot is a Suzuri rule, not an OS one: dot-entries are hidden from
    // the tree (VaultTreeModel), so a created or renamed ".foo" would land
    // on disk and immediately vanish from view
    if (name.startsWith(u'.')) {
        return Problem::LeadingDot;
    }

    if (name.endsWith(u'.') || name.endsWith(u' ')) {
        return Problem::TrailingDotOrSpace;
    }

    if (isReservedName_(name)) {
        return Problem::ReservedName;
    }

    return Problem::None;
}

[[nodiscard]] inline bool isValid(const QString& name)
{
    return problemOf(name) == Problem::None;
}

// The forbidden characters, spaced for a legible message (kept in sync with
// isForbiddenChar_'s switch by hand — the set is tiny and rarely changes, and
// describeInPath has the same list without "/")
[[nodiscard]] inline QString forbiddenCharsDisplay()
{
    return uR"(\ / : * ? " < > |)"_s;
}

// One advisory sentence for a problem, or empty for None / Empty (an empty
// field is a neutral state, not an error — the caller shows nothing). Phrased
// once here so create and rename read identically
[[nodiscard]] inline QString describe(Problem problem)
{
    switch (problem) {
    case Problem::ForbiddenChar:
        return QCoreApplication::translate(
                   "UserTextEntry",
                   "A name cannot contain any of:  %1")
            .arg(forbiddenCharsDisplay());
    case Problem::ReservedName:
        return QCoreApplication::translate(
            "UserTextEntry",
            "That is a Windows reserved name and cannot be used.");
    case Problem::LeadingDot:
        return QCoreApplication::translate(
            "UserTextEntry",
            "A name cannot start with a period.");
    case Problem::TrailingDotOrSpace:
        return QCoreApplication::translate(
            "UserTextEntry",
            "A name cannot end with a space or a period.");
    case Problem::None:
    case Problem::Empty:
        return {};
    }

    return {};
}

// describe for a name typed as one part of a path (Go to File), where "/"
// separates the names and so isn't listed as forbidden in one
[[nodiscard]] inline QString describeInPath(Problem problem)
{
    if (problem == Problem::ForbiddenChar) {
        return QCoreApplication::translate(
                   "UserTextEntry",
                   "A name cannot contain any of:  %1")
            .arg(uR"(\ : * ? " < > |)"_s);
    }

    return describe(problem);
}

} // namespace Suzuri::Ui::UserTextEntry
