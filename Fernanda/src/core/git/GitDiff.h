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

#include <optional>

#include <QByteArray>
#include <QByteArrayView>
#include <QChar>
#include <QList>
#include <QString>
#include <QStringView>
#include <QtMinMax>
#include <QtTypes>

#include "core/CoreConstants.h"

// One file's changes, read from what `git diff` prints in its unified format.
// Free functions with no process, so the reading is testable on its own;
// GitRepository runs git and asks.
//
// Only the hunks are read. The lines before the first one (diff --git, index,
// the --- and +++ names, mode changes) say what GitRepository already knows,
// apart from whether the file is binary, which git says there instead of
// giving hunks. A hunk's @@ line says how many lines it holds from each file,
// and the hunk is read to those counts: a context line starts with a space,
// a removed one with a minus, an added one with a plus, and git's "\ No
// newline at end of file" follows the line it's about. A line left empty is
// a blank context line, as git writes one under diff.suppressBlankEmpty.
// Counting, rather than stopping at the first line that doesn't fit, keeps
// that setting from cutting a hunk short.
//
// A line's text is UTF-8, without its line break. A carriage return before
// the break, from a file with Windows line endings, is dropped from every
// line, since the diff view shows lines, not their endings.
//
// Within a changed line, the words that changed are marked too (wordChanges),
// the way GitHub Desktop pairs lines: in a run of removed and added lines with
// as many of each, the first removed line goes with the first added one, and
// so on. A run with more of one than the other marks nothing, since there's no
// telling which line became which. Unlike GitHub Desktop, which marks one span
// from the first changed character to the last, a line can have any number of
// spans: in prose a paragraph is one line, and two small edits far apart
// would otherwise mark nearly all of it
namespace Fernanda::GitDiff {

// Part of a line's text, by QString index
struct Span
{
    qsizetype start = 0;
    qsizetype length = 0;

    friend bool operator==(const Span&, const Span&) = default;
};

struct Line
{
    enum class Kind
    {
        Context,
        Added,
        Removed,

        // Git's note that the line before it has no line break after it,
        // ending the file. It has no text: a view says it in its own words
        NoNewline
    };

    Kind kind = Kind::Context;
    QString text{};

    // Its number in the old file and in the new one, counted from 1. 0 where
    // the line isn't in that file: an added line has no old number, and a
    // removed one no new number. A NoNewline line has neither
    int oldNumber = 0;
    int newNumber = 0;

    // The parts of a removed or added line's text that changed, when it's
    // paired with a line of the other kind (wordChanges). Empty otherwise
    QList<Span> changes{};
};

struct Hunk
{
    // Where the hunk starts in each file, and how many of its lines it holds
    int oldStart = 0;
    int oldCount = 0;
    int newStart = 0;
    int newCount = 0;

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

// The changes between one line and the line it became, for each
struct WordChanges
{
    QList<Span> before{};
    QList<Span> after{};
};

namespace Internal {

// A word (letters, digits, and the accents on them), a run of whitespace, or
// any other single character. A character of a script written without spaces
// (Han, Hiragana, Katakana) is a word by itself, as the word count takes it
// (TextCounts)
struct Token_
{
    qsizetype start = 0;
    qsizetype length = 0;
    bool space = false;
};

enum class CharKind_
{
    Word,
    Solo,
    Space,
    Other
};

[[nodiscard]] inline CharKind_ charKindOf_(char32_t c)
{
    if (QChar::isSpace(c)) {
        return CharKind_::Space;
    }

    if (QChar::isLetterOrNumber(c)) {
        switch (QChar::script(c)) {
        case QChar::Script_Han:
        case QChar::Script_Hiragana:
        case QChar::Script_Katakana:
            return CharKind_::Solo;

        default:
            return CharKind_::Word;
        }
    }

    if (QChar::isMark(c)) {
        return CharKind_::Word;
    }

    return CharKind_::Other;
}

// The code point at i, and how many QChars it takes (two for a surrogate
// pair)
[[nodiscard]] inline char32_t
codePointAt_(QStringView text, qsizetype i, qsizetype& width)
{
    if (text[i].isHighSurrogate() && i + 1 < text.size() &&
        text[i + 1].isLowSurrogate()) {
        width = 2;
        return QChar::surrogateToUcs4(text[i], text[i + 1]);
    }

    width = 1;
    return text[i].unicode();
}

[[nodiscard]] inline QList<Token_> tokensOf_(QStringView text)
{
    QList<Token_> tokens{};
    qsizetype i = 0;

    while (i < text.size()) {
        qsizetype width = 0;
        auto kind = charKindOf_(codePointAt_(text, i, width));
        Token_ token{ i, width, kind == CharKind_::Space };

        // A word or a run of whitespace takes in the characters after it of
        // its kind
        while (i + token.length < text.size()) {
            qsizetype next_width = 0;
            auto next =
                charKindOf_(codePointAt_(text, i + token.length, next_width));

            if (next != kind || kind == CharKind_::Solo ||
                kind == CharKind_::Other) {
                break;
            }

            token.length += next_width;
        }

        tokens << token;
        i += token.length;
    }

    return tokens;
}

[[nodiscard]] inline QStringView viewOf_(QStringView text, const Token_& token)
{
    return text.sliced(token.start, token.length);
}

// How many characters of the tokens aren't whitespace
[[nodiscard]] inline qsizetype nonSpaceLength_(const QList<Token_>& tokens)
{
    qsizetype length = 0;

    for (const auto& token : tokens) {
        if (!token.space) {
            length += token.length;
        }
    }

    return length;
}

// The same, of the tokens not changed
[[nodiscard]] inline qsizetype unchangedNonSpaceLength_(
    const QList<Token_>& tokens,
    const QList<bool>& changed)
{
    qsizetype length = 0;

    for (qsizetype i = 0; i < tokens.size(); ++i) {
        if (!changed[i] && !tokens[i].space) {
            length += tokens[i].length;
        }
    }

    return length;
}

// An unchanged run of whitespace between two changed tokens is marked
// changed too, so a reworded phrase reads as one span
inline void joinAcrossSpaces_(const QList<Token_>& tokens, QList<bool>& changed)
{
    for (qsizetype i = 1; i + 1 < tokens.size(); ++i) {
        if (tokens[i].space && !changed[i] && changed[i - 1] &&
            changed[i + 1]) {
            changed[i] = true;
        }
    }
}

[[nodiscard]] inline QList<Span>
spansOf_(const QList<Token_>& tokens, const QList<bool>& changed)
{
    QList<Span> spans{};

    for (qsizetype i = 0; i < tokens.size(); ++i) {
        if (!changed[i]) {
            continue;
        }

        if (!spans.isEmpty() && i > 0 && changed[i - 1]) {
            spans.last().length += tokens[i].length;
        } else {
            spans << Span{ tokens[i].start, tokens[i].length };
        }
    }

    return spans;
}

// Marks which tokens in the middles (from `first` to `before_end` and
// `after_end`, the matching ends already trimmed) are in a longest run of
// tokens the two share; the rest are changed. A table of LCS lengths, read
// from the back so it can be walked from the front
inline void markMiddle_(
    QStringView before,
    const QList<Token_>& before_tokens,
    QStringView after,
    const QList<Token_>& after_tokens,
    qsizetype first,
    qsizetype before_end,
    qsizetype after_end,
    QList<bool>& before_changed,
    QList<bool>& after_changed)
{
    auto n = before_end - first;
    auto m = after_end - first;
    auto columns = m + 1;

    auto same = [&](qsizetype i, qsizetype j) {
        return viewOf_(before, before_tokens[first + i]) ==
               viewOf_(after, after_tokens[first + j]);
    };

    QList<int> table((n + 1) * columns, 0);

    for (auto i = n - 1; i >= 0; --i) {
        for (auto j = m - 1; j >= 0; --j) {
            table[i * columns + j] = same(i, j)
                                         ? table[(i + 1) * columns + j + 1] + 1
                                         : qMax(
                                               table[(i + 1) * columns + j],
                                               table[i * columns + j + 1]);
        }
    }

    qsizetype i = 0;
    qsizetype j = 0;

    while (i < n && j < m) {
        if (same(i, j)) {
            before_changed[first + i] = false;
            after_changed[first + j] = false;
            ++i;
            ++j;
        } else if (table[(i + 1) * columns + j] >= table[i * columns + j + 1]) {
            ++i;
        } else {
            ++j;
        }
    }
}

struct Range_
{
    int start = 0;
    int count = 0;
};

// "12,4" (a start, then a count), or "12", whose count is 1
[[nodiscard]] inline std::optional<Range_> rangeOf_(QByteArrayView range)
{
    auto comma = range.indexOf(',');
    auto start = comma < 0 ? range : range.first(comma);

    Range_ read{};
    auto ok = false;
    read.start = start.toInt(&ok);

    if (!ok) {
        return std::nullopt;
    }

    if (comma < 0) {
        read.count = 1;
        return read;
    }

    read.count = range.sliced(comma + 1).toInt(&ok);
    return ok ? std::optional<Range_>(read) : std::nullopt;
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

    auto old_range = rangeOf_(line.sliced(4, plus - 4));
    auto new_range = rangeOf_(line.sliced(plus + 2, end - plus - 2));

    if (!old_range || !new_range) {
        return std::nullopt;
    }

    Hunk hunk{};
    hunk.oldStart = old_range->start;
    hunk.oldCount = old_range->count;
    hunk.newStart = new_range->start;
    hunk.newCount = new_range->count;
    hunk.header = QString::fromUtf8(line);
    return hunk;
}

// A line's text, after its first character. An empty line has none
[[nodiscard]] inline QString textOf_(QByteArrayView line)
{
    return line.isEmpty() ? QString{} : QString::fromUtf8(line.sliced(1));
}

} // namespace Internal

// What changed between before and after, a line and the line it became, as
// spans of each: the words not in a longest run of words the two share. A
// run of whitespace between two changed words is marked with them.
//
// Nothing is marked when most of the line changed: when less than
// GIT_WORD_DIFF_MIN_UNCHANGED_PERCENT of the longer line's characters,
// leaving out whitespace, are unchanged. A rewritten line still shares a few
// words ("the", a comma) with the old one, and marking around them shows
// nothing the line's own tint doesn't.
//
// The words the lines start and end with in common are set aside first, so
// only the middles are compared. A middle too large to compare
// (GIT_WORD_DIFF_MAX_CELLS) is marked as one span
[[nodiscard]] inline WordChanges
wordChanges(const QString& before, const QString& after)
{
    auto before_tokens = Internal::tokensOf_(before);
    auto after_tokens = Internal::tokensOf_(after);

    auto same = [&](qsizetype i, qsizetype j) {
        return Internal::viewOf_(before, before_tokens[i]) ==
               Internal::viewOf_(after, after_tokens[j]);
    };

    auto shorter = qMin(before_tokens.size(), after_tokens.size());

    qsizetype first = 0;
    while (first < shorter && same(first, first)) {
        ++first;
    }

    auto before_end = before_tokens.size();
    auto after_end = after_tokens.size();
    while (before_end > first && after_end > first &&
           same(before_end - 1, after_end - 1)) {
        --before_end;
        --after_end;
    }

    // Everything between the matching ends is changed, until a match in the
    // middles says otherwise
    QList<bool> before_changed(before_tokens.size(), false);
    QList<bool> after_changed(after_tokens.size(), false);

    for (auto i = first; i < before_end; ++i) {
        before_changed[i] = true;
    }

    for (auto j = first; j < after_end; ++j) {
        after_changed[j] = true;
    }

    auto cells = qint64(before_end - first) * qint64(after_end - first);

    if (cells > 0 && cells <= GIT_WORD_DIFF_MAX_CELLS) {
        Internal::markMiddle_(
            before,
            before_tokens,
            after,
            after_tokens,
            first,
            before_end,
            after_end,
            before_changed,
            after_changed);
    }

    auto unchanged =
        Internal::unchangedNonSpaceLength_(before_tokens, before_changed);
    auto longer = qMax(
        Internal::nonSpaceLength_(before_tokens),
        Internal::nonSpaceLength_(after_tokens));

    if (unchanged * 100 < longer * GIT_WORD_DIFF_MIN_UNCHANGED_PERCENT) {
        return {};
    }

    Internal::joinAcrossSpaces_(before_tokens, before_changed);
    Internal::joinAcrossSpaces_(after_tokens, after_changed);

    return { Internal::spansOf_(before_tokens, before_changed),
             Internal::spansOf_(after_tokens, after_changed) };
}

namespace Internal {

// Pairs the removed and added lines in each run of them (a NoNewline note
// doesn't end a run), when a run has as many of each, and marks each pair's
// changes
inline void markWordChanges_(Hunk& hunk)
{
    QList<qsizetype> removed{};
    QList<qsizetype> added{};

    auto pair_run = [&] {
        if (removed.size() == added.size()) {
            for (qsizetype k = 0; k < removed.size(); ++k) {
                auto& old_line = hunk.lines[removed[k]];
                auto& new_line = hunk.lines[added[k]];
                auto changes = wordChanges(old_line.text, new_line.text);
                old_line.changes = changes.before;
                new_line.changes = changes.after;
            }
        }

        removed.clear();
        added.clear();
    };

    for (qsizetype i = 0; i < hunk.lines.size(); ++i) {
        switch (hunk.lines[i].kind) {
        case Line::Kind::Removed:
            removed << i;
            break;
        case Line::Kind::Added:
            added << i;
            break;
        case Line::Kind::NoNewline:
            break;
        case Line::Kind::Context:
            pair_run();
            break;
        }
    }

    pair_run();
}

} // namespace Internal

[[nodiscard]] inline Diff parse(const QByteArray& output)
{
    Diff diff{};

    // The hunk being read, the next line numbers in it, and how many lines
    // of each file it still holds
    Hunk* hunk = nullptr;
    auto old_number = 0;
    auto new_number = 0;
    auto old_left = 0;
    auto new_left = 0;

    for (QByteArrayView line : output.split('\n')) {
        if (line.endsWith('\r')) {
            line.chop(1);
        }

        if (hunk) {
            // Git's note can follow the hunk's last line, after the counts
            // have run out
            if (line.startsWith('\\')) {
                hunk->lines << Line{ Line::Kind::NoNewline, {}, 0, 0 };
                continue;
            }

            auto is_context = line.isEmpty() || line.startsWith(' ');

            if (is_context && old_left > 0 && new_left > 0) {
                hunk->lines << Line{ Line::Kind::Context,
                                     Internal::textOf_(line),
                                     old_number++,
                                     new_number++ };
                --old_left;
                --new_left;
                continue;
            }

            if (line.startsWith('-') && old_left > 0) {
                hunk->lines << Line{ Line::Kind::Removed,
                                     Internal::textOf_(line),
                                     old_number++,
                                     0 };
                --old_left;
                continue;
            }

            if (line.startsWith('+') && new_left > 0) {
                hunk->lines << Line{ Line::Kind::Added,
                                     Internal::textOf_(line),
                                     0,
                                     new_number++ };
                --new_left;
                continue;
            }

            hunk = nullptr;
        }

        if (auto next = Internal::hunkOf_(line)) {
            diff.hunks << *next;
            hunk = &diff.hunks.last();
            old_number = hunk->oldStart;
            new_number = hunk->newStart;
            old_left = hunk->oldCount;
            new_left = hunk->newCount;
            continue;
        }

        if (diff.hunks.isEmpty() && line.startsWith("Binary files ")) {
            diff.binary = true;
        }
    }

    for (auto& each : diff.hunks) {
        Internal::markWordChanges_(each);
    }

    return diff;
}

} // namespace Fernanda::GitDiff
