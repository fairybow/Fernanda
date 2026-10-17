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
#include <QString>
#include <QStringView>
#include <QtTypes>

#include "core/spell/SpellChecker.h"
#include "core/spell/WordList.h"

// Whether a word is misspelled, given a dictionary and the words to take as
// correct besides. Free functions in core, so the rule is in one place, for
// the underlines and the context menu alike, and can be tested without an
// editor.
//
// A word is one as SpellWords::find gives it. It is correctly spelled when
// the checker or the accepted words take it whole. The checker is asked
// first: it remembers its answers, and most words are in the dictionary.
//
// Failing that, two forms the checker can't judge for itself, since it doesn't
// hold the accepted words:
// - A possessive of an accepted word: "Pangloss's" is correct when "Pangloss"
//   is accepted. Only the accepted words are tried with the 's removed. The
//   checker has the dictionary's own rule for which of its words take one
// - A hyphenated word with an accepted word among its parts: "Pangloss-like"
//   is correct when each part is, by any of the above. The checker splits a
//   hyphenated word too, but tries each part against the dictionary alone
namespace Suzuri::Misspelling {

namespace Internal {

[[nodiscard]] inline bool isApostrophe_(QChar c)
{
    return c == QChar(u'\'') || c == QChar(0x2019);
}

// The hyphens SpellWords keeps inside a word
[[nodiscard]] inline bool isHyphen_(QChar c)
{
    return c == QChar(u'-') || c == QChar(0x2010) || c == QChar(0x2011);
}

[[nodiscard]] inline bool hasPossessive_(QStringView word)
{
    auto size = word.size();

    return size > 2 && word[size - 1].toLower() == QChar(u's') &&
           isApostrophe_(word[size - 2]);
}

// A whole word, or one part of a hyphenated one
[[nodiscard]] inline bool isCorrect_(
    SpellChecker& checker,
    const WordList& acceptedWords,
    const QString& word)
{
    if (checker.isCorrect(word) || acceptedWords.contains(word)) {
        return true;
    }

    return hasPossessive_(word) && acceptedWords.contains(word.chopped(2));
}

} // namespace Internal

// The word without a possessive 's (or 'S, or either with a typographic
// apostrophe) at its end. This is the form to add to a word list: the
// possessive is then correct too, where adding the possessive would leave
// the word itself misspelled
[[nodiscard]] inline QString withoutPossessive(const QString& word)
{
    return Internal::hasPossessive_(word) ? word.chopped(2) : word;
}

[[nodiscard]] inline bool isMisspelled(
    SpellChecker& checker,
    const WordList& acceptedWords,
    const QString& word)
{
    if (Internal::isCorrect_(checker, acceptedWords, word)) {
        return false;
    }

    // Each part between hyphens. A word with no hyphen is its own one part,
    // which has just failed
    qsizetype start = 0;
    auto hyphenated = false;

    for (qsizetype i = 0; i <= word.size(); ++i) {
        if (i < word.size() && !Internal::isHyphen_(word[i])) {
            continue;
        }

        if (i < word.size()) {
            hyphenated = true;
        }

        if (!hyphenated || !Internal::isCorrect_(
                               checker,
                               acceptedWords,
                               word.mid(start, i - start))) {
            return true;
        }

        start = i + 1;
    }

    return false;
}

} // namespace Suzuri::Misspelling
