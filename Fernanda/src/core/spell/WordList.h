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
#include <QChar>
#include <QSet>
#include <QString>
#include <QStringList>

#include <Coco/Debug.h>
#include <Coco/Path.h>

#include "core/Io.h"

namespace Fernanda {

using namespace Qt::StringLiterals;

// A set of words to take as correctly spelled: a vault's own dictionary, the
// words ignored for now, or several of those together. QtCore-only.
//
// contains follows Hunspell's rule for a word added to a dictionary. A word
// stored in lowercase also matches its capitalized and all-caps forms, so
// "pangloss" accepts "Pangloss" at the start of a sentence. A word stored with
// capitals matches only itself and its all-caps form: "Pangloss" accepts
// "PANGLOSS" but not "pangloss".
//
// A typographic apostrophe is stored and matched as a plain one, as
// SpellChecker reads it.
//
// The file form is UTF-8, one word per line, sorted. Blank lines and space
// around a word are ignored when it is read, so a list edited by hand needn't
// be tidy
class WordList
{
public:
    WordList() = default;

    [[nodiscard]] bool isEmpty() const noexcept { return words_.isEmpty(); }
    [[nodiscard]] qsizetype size() const noexcept { return words_.size(); }

    [[nodiscard]] bool operator==(const WordList& other) const
    {
        return words_ == other.words_;
    }

    [[nodiscard]] bool contains(const QString& word) const
    {
        auto plain = plain_(word);

        if (plain.isEmpty()) {
            return false;
        }

        if (words_.contains(plain)) {
            return true;
        }

        auto upper = plain.toUpper();

        // All caps: matches any stored word in any case
        if (plain == upper && plain != plain.toLower()) {
            return uppers_.contains(plain);
        }

        // Capitalized: matches the word stored in lowercase
        if (plain.front().isUpper()) {
            auto lowered = plain;
            lowered.front() = lowered.front().toLower();

            return lowered == lowered.toLower() && words_.contains(lowered);
        }

        return false;
    }

    // Returns whether the word was new. An empty word is never added
    bool insert(const QString& word)
    {
        auto plain = plain_(word).trimmed();

        if (plain.isEmpty() || words_.contains(plain)) {
            return false;
        }

        words_.insert(plain);
        uppers_.insert(plain.toUpper());

        return true;
    }

    void unite(const WordList& other)
    {
        words_.unite(other.words_);
        uppers_.unite(other.uppers_);
    }

    // Sorted by code point, so the file keeps the same order from one write
    // to the next
    [[nodiscard]] QStringList words() const
    {
        QStringList sorted(words_.cbegin(), words_.cend());
        sorted.sort();

        return sorted;
    }

    // An empty list for a file that doesn't exist; nullopt for one that does
    // but couldn't be read, so a caller about to write the file back can tell
    // "nothing yet" from "something it mustn't overwrite"
    [[nodiscard]] static std::optional<WordList> read(const Coco::Path& file)
    {
        if (!file.exists()) {
            return WordList{};
        }

        auto bytes = Io::tryRead(file);
        if (!bytes) {
            return std::nullopt;
        }

        WordList list{};

        for (const auto& line : QString::fromUtf8(*bytes).split(u'\n')) {
            list.insert(line);
        }

        return list;
    }

    // Makes no folders. Returns false if the write failed — already logged
    bool write(const Coco::Path& file) const
    {
        QByteArray bytes{};

        for (const auto& word : words()) {
            bytes += word.toUtf8() + '\n';
        }

        return Io::write(bytes, file, Io::CreateDirs::No);
    }

private:
    QSet<QString> words_{};

    // Every word in all caps, for matching an all-caps word in any case
    QSet<QString> uppers_{};

    [[nodiscard]] static QString plain_(const QString& word)
    {
        auto plain = word;
        plain.replace(QChar(0x2019), QChar(u'\''));

        return plain;
    }
};

} // namespace Fernanda
