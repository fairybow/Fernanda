/*
 * Suzuri — A plain-text writing app for fiction, poetry, and long projects
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
#include <QList>
#include <QString>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QtTypes>

// Finding text in a document, and replacing what was found. Free functions, so
// what counts as a match is defined once and can be tested without an editor.
//
// Every function works on the QTextDocument it is given, and a view gives its
// own: an edit made there reaches the buffer and the other views as typing
// does (see docs/Architecture.md, "The prime document").
//
// The term is literal text. A match lies within one line, and matches never
// overlap: the search for the next one starts where the last one ended.
//
// A space and a no-break space match each other, in either direction, so a
// phrase is found whichever of the two its words are joined with. Replacing
// writes the replacement as given
namespace Suzuri::TextSearch {

struct Options
{
    bool matchCase = false;

    // The match may not touch a letter or a digit on either side. Anything
    // else is a boundary, an apostrophe included: "don" is a whole word in
    // "don't"
    bool wholeWord = false;
};

// A match's place in its document, in the units QTextCursor positions use
struct Match
{
    int position = 0;
    int length = 0;
};

namespace Internal {

// Whether text[start, end) touches no letter or digit on either side
[[nodiscard]] inline bool
isWholeWord_(const QString& text, qsizetype start, qsizetype end)
{
    if (start > 0 && text.at(start - 1).isLetterOrNumber()) {
        return false;
    }

    return end >= text.size() || !text.at(end).isLetterOrNumber();
}

} // namespace Internal

// Every match in the document, in order. An empty term matches nothing.
//
// Searches each line's text with QString::indexOf. QTextDocument::find gives
// the same matches, but builds a cursor for each one and takes a hundred
// times as long over a common word in a long file
[[nodiscard]] inline QList<Match>
findAll(const QTextDocument* document, const QString& term, Options options)
{
    QList<Match> matches{};

    if (term.isEmpty()) {
        return matches;
    }

    auto spaced_term = term;
    spaced_term.replace(QChar::Nbsp, QChar::Space);

    auto length = spaced_term.size();
    auto sensitivity =
        options.matchCase ? Qt::CaseSensitive : Qt::CaseInsensitive;

    for (auto block = document->begin(); block.isValid();
         block = block.next()) {
        auto text = block.text();
        text.replace(QChar::Nbsp, QChar::Space);

        qsizetype from = 0;

        while (true) {
            auto index = text.indexOf(spaced_term, from, sensitivity);
            if (index < 0) {
                break;
            }

            if (options.wholeWord &&
                !Internal::isWholeWord_(text, index, index + length)) {
                from = index + 1;
                continue;
            }

            matches << Match{ block.position() + static_cast<int>(index),
                              static_cast<int>(length) };
            from = index + length;
        }
    }

    return matches;
}

// The index of the first match at or after position, or of the first match of
// all when there is none: going forward from the last match wraps to the top.
// -1 when there are no matches
[[nodiscard]] inline int nextIndex(const QList<Match>& matches, int position)
{
    if (matches.isEmpty()) {
        return -1;
    }

    for (auto i = 0; i < matches.size(); ++i) {
        if (matches.at(i).position >= position) {
            return i;
        }
    }

    return 0;
}

// The index of the last match that starts before position, or of the last
// match of all when there is none: going back from the first match wraps to
// the bottom. -1 when there are no matches
[[nodiscard]] inline int
previousIndex(const QList<Match>& matches, int position)
{
    if (matches.isEmpty()) {
        return -1;
    }

    for (auto i = matches.size() - 1; i >= 0; --i) {
        if (matches.at(i).position < position) {
            return static_cast<int>(i);
        }
    }

    return static_cast<int>(matches.size() - 1);
}

// Replace one match's text. The match must be current: found in this document
// since its last change
inline void replaceOne(
    QTextDocument* document,
    const Match& match,
    const QString& replacement)
{
    QTextCursor cursor(document);
    cursor.setPosition(match.position);
    cursor.setPosition(match.position + match.length, QTextCursor::KeepAnchor);
    cursor.insertText(replacement);
}

// Replace every match, and return how many were replaced. A match whose text
// already equals the replacement is left alone and not counted, so replacing
// "the" with "the" changes nothing and leaves the document unmodified.
//
// All of it is one edit block, which a document reports as one change. Made in
// a view's document, that reaches the buffer as one edit and undoes as one
// step. The matches are found first and replaced from the last to the first,
// so replacing one never moves another, and text put in by a replacement is
// never searched
inline int replaceAll(
    QTextDocument* document,
    const QString& term,
    const QString& replacement,
    Options options)
{
    const auto matches = findAll(document, term, options);
    auto replaced = 0;

    QTextCursor cursor(document);
    cursor.beginEditBlock();

    for (auto i = matches.size() - 1; i >= 0; --i) {
        const auto& match = matches.at(i);

        cursor.setPosition(match.position);
        cursor.setPosition(
            match.position + match.length,
            QTextCursor::KeepAnchor);

        if (cursor.selectedText() == replacement) {
            continue;
        }

        cursor.insertText(replacement);
        ++replaced;
    }

    cursor.endEditBlock();
    return replaced;
}

} // namespace Suzuri::TextSearch
