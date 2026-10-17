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

#include <QFile>
#include <QFileDevice>
#include <QHash>
#include <QString>
#include <QStringList>

#include <Coco/Debug.h>
#include <Coco/Path.h>

#include "core/spell/SpellChecker.h"

namespace Fernanda {

using namespace Qt::StringLiterals;

// The spelling dictionaries in one folder, and a checker for each that has
// been asked for.
//
// A dictionary is two files with one name, as Hunspell and LibreOffice
// publish them: en_US.aff and en_US.dic. The name is the language, and any
// pair in the folder is offered, so dropping two files in adds a language.
//
// A checker is built the first time its language is asked for, since loading
// a dictionary takes a moment and several megabytes, and is kept: every
// vault using a language shares one. This class owns them
class SpellCheckers
{
public:
    explicit SpellCheckers(const Coco::Path& folder)
        : folder_(folder)
    {
    }

    ~SpellCheckers() { qDeleteAll(checkers_); }

    SpellCheckers(const SpellCheckers&) = delete;
    SpellCheckers& operator=(const SpellCheckers&) = delete;

    [[nodiscard]] Coco::Path folder() const { return folder_; }

    // Copy each dictionary file compiled into the app (resources/
    // Dictionaries.qrc) into folder, unless a file of that name is already
    // there. Hunspell reads its dictionaries from disk, so they can't stay in
    // the resources. A file already in the folder is never replaced: it may
    // be a newer one the user put there.
    //
    // A copy of a resource is read-only, as the resource is, so each is made
    // writable: the folder is the user's to manage
    static void installBundled(const Coco::Path& folder)
    {
        if (!folder.isDir() && !Coco::mkpath(folder)) {
            WARN("Can't make the dictionaries folder {}", folder);
            return;
        }

        const auto bundled = Coco::allFilePaths(
            Coco::Path(":/dictionaries"),
            { u"*.aff"_s, u"*.dic"_s });

        for (const auto& source : bundled) {
            auto target = folder / source.name();
            if (target.exists()) {
                continue;
            }

            if (!QFile::copy(source.toQString(), target.toQString())) {
                WARN("Can't copy the dictionary file {} to {}", source, target);
                continue;
            }

            QFile::setPermissions(
                target.toQString(),
                QFileDevice::ReadOwner | QFileDevice::WriteOwner |
                    QFileDevice::ReadGroup | QFileDevice::ReadOther);
        }
    }

    // The languages with both files in the folder, by name, in order. Read
    // from the folder on each call, so a dictionary added while Fernanda runs
    // is found
    [[nodiscard]] QStringList languages() const
    {
        QStringList result{};

        if (!folder_.isDir()) {
            return result;
        }

        const auto affix_files = Coco::filePaths(folder_, { u"*.aff"_s });

        for (const auto& affix_file : affix_files) {
            auto language = affix_file.stemQString();

            if (wordFile_(language).isFile()) {
                result << language;
            }
        }

        result.sort();
        return result;
    }

    // Whether a language's dictionary can be checked against: both its files
    // are in the folder, and they are in an encoding Qt converts here
    // (SpellChecker::canConvert). The checker for one that can't is not
    // valid, and marks nothing. Reads the affix file's lines each call and
    // loads no dictionary
    [[nodiscard]] bool isUsable(const QString& language) const
    {
        if (!isName_(language)) {
            return false;
        }

        auto affix_file = affixFile_(language);

        return affix_file.isFile() && wordFile_(language).isFile() &&
               SpellChecker::canConvert(
                   SpellChecker::declaredEncoding(affix_file));
    }

    // The checker for a language, or nullptr when the folder has no
    // dictionary of that name, or the language isn't a single name. The
    // pointer is good for as long as this object is
    [[nodiscard]] SpellChecker* checker(const QString& language)
    {
        if (!isName_(language)) {
            WARN("Not a dictionary name: {}", language);
            return nullptr;
        }

        if (auto it = checkers_.constFind(language);
            it != checkers_.constEnd()) {
            return it.value();
        }

        if (!affixFile_(language).isFile() || !wordFile_(language).isFile()) {
            return nullptr;
        }

        auto* checker =
            new SpellChecker(affixFile_(language), wordFile_(language));
        checkers_.insert(language, checker);

        return checker;
    }

private:
    Coco::Path folder_;
    QHash<QString, SpellChecker*> checkers_{};

    // A language becomes a file name in the folder, and comes from a vault's
    // settings file, which can be edited by hand. So it must be one name, as a
    // vault takes a new name for an entry (Vault::rename): with a separator, a
    // "." or "..", or a root, it would name a file elsewhere
    [[nodiscard]] static bool isName_(const QString& language)
    {
        auto name = Coco::Path(language);
        return !name.isEmpty() && name.isPlain() && name.name() == name;
    }

    [[nodiscard]] Coco::Path affixFile_(const QString& language) const
    {
        return folder_ / (language + u".aff"_s);
    }

    [[nodiscard]] Coco::Path wordFile_(const QString& language) const
    {
        return folder_ / (language + u".dic"_s);
    }
};

} // namespace Fernanda
