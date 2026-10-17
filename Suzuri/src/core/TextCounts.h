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
#include <QStringView>
#include <QTextBoundaryFinder>

// What Suzuri counts in a piece of text, and how. Free functions in core, so
// the status bar's items (ui/WordCounter.h, ui/CursorPosition.h) share one
// definition and it can be read in one place. QtCore-only.
//
// Every function takes ONE LINE's text (a QTextBlock's), never text holding
// line breaks: a word or a character never spans a line, so a document's
// count is the sum of its lines' counts, and callers walk the blocks
namespace Suzuri::TextCounts {

namespace Internal {

// What a code point is to the word scanner (wordCount)
enum class Kind_
{
    Letter, // continues a word, or starts one
    Digit,  // as Letter, and the only thing a comma joins
    Mark,   // an accent on the character before it: never starts a word
    Solo,   // a word by itself (see wordCount, on CJK)
    Joiner, // stays inside a word when it sits between two word characters
    Comma,  // a Joiner between two digits only
    Other   // everything else: ends a word
};

[[nodiscard]] inline Kind_ kindOf_(char32_t c)
{
    switch (c) {
    case U'\'':     // apostrophe
    case U'\u2019': // right single quote, the typeset apostrophe
    case U'-':      // hyphen-minus
    case U'\u2010': // hyphen
    case U'\u2011': // non-breaking hyphen
    case U'\u00AD': // soft hyphen
    case U'.':
        return Kind_::Joiner;

    case U',':
        return Kind_::Comma;

    default:
        break;
    }

    if (QChar::isDigit(c)) {
        return Kind_::Digit;
    }

    if (QChar::isLetterOrNumber(c)) {
        switch (QChar::script(c)) {
        case QChar::Script_Han:
        case QChar::Script_Hiragana:
        case QChar::Script_Katakana:
            return Kind_::Solo;

        default:
            return Kind_::Letter;
        }
    }

    if (QChar::isMark(c)) {
        return Kind_::Mark;
    }

    // The underscore and its kin, so snake_case is one word
    if (QChar::category(c) == QChar::Punctuation_Connector) {
        return Kind_::Letter;
    }

    return Kind_::Other;
}

} // namespace Internal

// The characters in text, as a reader counts them: one per grapheme cluster
// (Unicode's "user-perceived character"), spaces and tabs included. So an
// emoji is one, and so is an "é" stored as "e" plus a combining accent —
// QString::size() calls each of those two, since it counts UTF-16 code units.
// The cursor's column is in the same unit (CursorPosition)
[[nodiscard]] inline int characterCount(QStringView text)
{
    QTextBoundaryFinder finder(QTextBoundaryFinder::Grapheme, text);
    auto count = 0;

    // Every cluster ends at a boundary, the end of the text included, and the
    // finder returns -1 once it's past the last one
    while (finder.toNextBoundary() != -1) {
        ++count;
    }

    return count;
}

// The words in text, as a writer counts them. One pass, nothing allocated.
//
// A word is a run of letters and digits (with their accents). A few
// characters stay inside a word when they sit BETWEEN two of its characters —
// apostrophes, hyphens, and the period — and a comma does between two digits.
// So "don't", "well-known", "e.g." and "1,000" are one word each. Everything
// else separates: whitespace, dashes, slashes, an ellipsis, and two joiners
// in a row ("word--word", "wait...what"). Only letters and digits make a word,
// so Markdown and Fountain marks never count: "# Heading" is one word,
// "**bold**" is one, and a lone "-" or "—" is none.
//
// Splitting on whitespace alone counts those marks as words and reads
// "word—word" as one. Unicode's own word boundaries (QTextBoundaryFinder::Word)
// read "well-known" as two, which no word processor does.
//
// Chinese characters and Japanese kana are one word EACH, since those scripts
// put no spaces between words — Obsidian's rule. Korean uses spaces and
// counts as runs. Known limit: Thai, Lao, Khmer and Myanmar also write
// without spaces and have no such rule here, so they undercount
[[nodiscard]] inline int wordCount(QStringView text)
{
    using Internal::Kind_;

    auto count = 0;
    auto in_word = false;        // the last character belongs to a word
    auto joiner_pending = false; // ...and was a joiner, not yet between two
    auto comma_pending = false;  // ...and that joiner was a comma
    auto after_digit = false;    // the word's last letter-or-digit was a digit

    for (qsizetype i = 0; i < text.size(); ++i) {
        char32_t c = text[i].unicode();

        if (text[i].isHighSurrogate() && i + 1 < text.size() &&
            text[i + 1].isLowSurrogate()) {
            c = QChar::surrogateToUcs4(text[i], text[i + 1]);
            ++i;
        }

        auto kind = Internal::kindOf_(c);

        switch (kind) {
        case Kind_::Letter:
        case Kind_::Digit: {
            auto is_digit = (kind == Kind_::Digit);

            if (!in_word) {
                ++count;
                in_word = true;
            } else if (
                joiner_pending && comma_pending && !(after_digit && is_digit)) {
                // A comma that wasn't between two digits separates after
                // all: "a,b" is two words
                ++count;
            }

            joiner_pending = false;
            after_digit = is_digit;
            break;
        }

        case Kind_::Mark:
            // Part of the character before it. After a joiner there is no
            // such character, so the word is over
            if (joiner_pending) {
                in_word = false;
                joiner_pending = false;
            }
            break;

        case Kind_::Solo:
            ++count;
            in_word = false;
            joiner_pending = false;
            break;

        case Kind_::Joiner:
        case Kind_::Comma:
            if (in_word && !joiner_pending) {
                // Held until the next character says whether it joined
                joiner_pending = true;
                comma_pending = (kind == Kind_::Comma);
            } else {
                in_word = false;
                joiner_pending = false;
            }
            break;

        case Kind_::Other:
            in_word = false;
            joiner_pending = false;
            break;
        }
    }

    return count;
}

} // namespace Suzuri::TextCounts
