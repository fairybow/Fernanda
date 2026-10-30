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

#include <QChar>
#include <QList>
#include <QStringView>
#include <QtTypes>

// Which stretches of a line of text are words to check the spelling of. Free
// functions in core, so the rule is in one place and can be tested without an
// editor. QtCore-only.
//
// A word is a run of letters and digits, with their accents. An apostrophe
// (plain or typographic) or a hyphen stays inside a word when it sits BETWEEN
// two of its characters, so "don't" and "well-known" are one word each, while
// the quotation marks around 'this' and a dash - like that one - are not part
// of any. Everything else separates words, the underscore and the period
// included.
//
// Chinese characters and Japanese kana are never part of a word here: those
// scripts put no spaces between words, and a dictionary of this kind can't
// check them.
//
// Digits are kept in a word so that "3rd" and "mp3" come out whole;
// SpellChecker then leaves a word with a digit in it alone.
//
// This is not TextCounts::wordCount's rule, though it is close. A count wants
// "e.g." and snake_case as one word each; a spelling check wants their parts
namespace Fernanda::SpellWords {

// A word's place in its line, in UTF-16 code units, as QString indexes them
struct Range
{
    qsizetype start = 0;
    qsizetype length = 0;
};

namespace Internal {

enum class Kind_
{
    Word,   // a letter, a digit, or an accent on one
    Joiner, // stays inside a word when it sits between two word characters
    Other   // everything else: ends a word
};

[[nodiscard]] inline Kind_ kindOf_(char32_t c)
{
    switch (c) {
    case U'\'':  // apostrophe
    case 0x2019: // right single quote, the typeset apostrophe
    case U'-':   // hyphen-minus
    case 0x2010: // hyphen
    case 0x2011: // non-breaking hyphen
        return Kind_::Joiner;

    default:
        break;
    }

    if (QChar::isLetterOrNumber(c)) {
        switch (QChar::script(c)) {
        case QChar::Script_Han:
        case QChar::Script_Hiragana:
        case QChar::Script_Katakana:
            return Kind_::Other;

        default:
            return Kind_::Word;
        }
    }

    if (QChar::isMark(c)) {
        return Kind_::Word;
    }

    return Kind_::Other;
}

} // namespace Internal

// Every word in one line's text, in order. The text holds no line break
[[nodiscard]] inline QList<Range> find(QStringView line)
{
    using Internal::Kind_;

    QList<Range> words{};

    qsizetype start = -1; // where the word being read began, or -1 for none
    qsizetype end = 0;    // just past its last word character

    auto close = [&] {
        if (start >= 0) {
            words << Range{ start, end - start };
            start = -1;
        }
    };

    // A joiner is held back until the character after it shows whether it
    // was between two word characters
    auto joiner_pending = false;

    for (qsizetype i = 0; i < line.size(); ++i) {
        auto at = i;
        char32_t c = line[i].unicode();

        if (line[i].isHighSurrogate() && i + 1 < line.size() &&
            line[i + 1].isLowSurrogate()) {
            c = QChar::surrogateToUcs4(line[i], line[i + 1]);
            ++i;
        }

        switch (Internal::kindOf_(c)) {
        case Kind_::Word:
            if (start < 0) {
                start = at;
            }

            end = i + 1;
            joiner_pending = false;
            break;

        case Kind_::Joiner:
            // A second joiner in a row, or one with no word before it,
            // joins nothing
            if (start < 0 || joiner_pending) {
                close();
                joiner_pending = false;
            } else {
                joiner_pending = true;
            }
            break;

        case Kind_::Other:
            close();
            joiner_pending = false;
            break;
        }
    }

    close();
    return words;
}

} // namespace Fernanda::SpellWords
