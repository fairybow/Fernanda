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

#include <functional>

#include <QCoreApplication>
#include <QList>
#include <QMessageBox>
#include <QString>
#include <QStringList>
#include <QWidget>

#include <Coco/Path.h>

#include "core/files/Docx.h"
#include "core/files/FileTypes.h"
#include "core/files/ImportTypes.h"
#include "core/files/PathKey.h"
#include "core/vault/Vault.h"

namespace Fernanda::Ui {

using namespace Qt::StringLiterals;

// Where Import's file dialog starts, and where it reports the files were
// chosen from. App's (AppConfig::importMruDir), handed down to every window
// that imports, so each import starts where the last one in any window left
// off
struct ImportFolder
{
    std::function<Coco::Path()> get{};
    std::function<void(const Coco::Path&)> set{};
};

namespace Internal {

// One file's import: the key of the file it made, or an empty key and the
// message saying why it made none
struct Imported_
{
    PathKey key{};
    QString failure{};
};

// The name a converted type goes by in the dialog's filter
[[nodiscard]] inline QString convertedTypeName_(ImportTypes::Type type)
{
    switch (type) {
    case ImportTypes::Type::Docx:
        return QCoreApplication::translate("FileImport", "Word documents");
    case ImportTypes::Type::Copy:
    case ImportTypes::Type::Unsupported:
        break;
    }

    return {};
}

// The name a type Fernanda opens, and so copies, goes by in the filter
[[nodiscard]] inline QString copiedTypeName_(FileTypes::Type type)
{
    switch (type) {
    case FileTypes::Type::Text:
        return QCoreApplication::translate("FileImport", "Text files");
    case FileTypes::Type::Pdf:
        return QCoreApplication::translate("FileImport", "PDFs");
    case FileTypes::Type::Image:
        return QCoreApplication::translate("FileImport", "Images");
    case FileTypes::Type::Unsupported:
        break;
    }

    return {};
}

// "Name (*.a *.b)"
[[nodiscard]] inline QString
filterEntry_(const QString& name, const QStringList& patterns)
{
    return name + u" ("_s + patterns.join(u' ') + u')';
}

// Every importable file first, then each converted type, then each type
// copied: "Importable files (*.docx *.txt ...);;Word documents (*.docx);;Text
// files (*.txt *.md ...);;PDFs (*.pdf);;Images (*.png ...)"
[[nodiscard]] inline QString importFilter_()
{
    QStringList every_pattern{};
    QStringList filters{};

    for (auto type : ImportTypes::types()) {
        auto patterns = ImportTypes::patternsOf(type);
        every_pattern << patterns;
        filters << filterEntry_(convertedTypeName_(type), patterns);
    }

    for (auto type : FileTypes::types()) {
        auto patterns = FileTypes::patternsOf(type);
        every_pattern << patterns;
        filters << filterEntry_(copiedTypeName_(type), patterns);
    }

    filters.prepend(filterEntry_(
        QCoreApplication::translate("FileImport", "Importable files"),
        every_pattern));

    return filters.join(u";;"_s);
}

[[nodiscard]] inline QString notSaved_(const Coco::Path& path)
{
    return QCoreApplication::translate(
               "FileImport",
               "\"%1\" couldn't be saved in the vault.")
        .arg(path.nameQString());
}

// A Word document, read as plain text and written into dir as a .txt file
[[nodiscard]] inline Imported_
importDocx_(Vault* vault, const PathKey& dir, const Coco::Path& path)
{
    auto conversion = Docx::toPlainText(path);
    auto name = path.nameQString();

    switch (conversion.failure) {
    case Docx::Failure::None:
        break;

    case Docx::Failure::NotAWordDocument:
        return { {},
                 QCoreApplication::translate(
                     "FileImport",
                     "\"%1\" isn't a Word document Fernanda can read.")
                     .arg(name) };

    case Docx::Failure::Damaged:
        return { {},
                 QCoreApplication::translate("FileImport", "\"%1\" is damaged.")
                     .arg(name) };
    }

    auto key = vault->createFile(dir, path.stemQString(), conversion.text);

    if (key.isEmpty()) {
        return { {}, notSaved_(path) };
    }

    return { key, {} };
}

// A file Fernanda opens, copied into dir unchanged
[[nodiscard]] inline Imported_
importCopy_(Vault* vault, const PathKey& dir, const Coco::Path& path)
{
    auto key = vault->copyFile(dir, path);

    if (key.isEmpty()) {
        return { {}, notSaved_(path) };
    }

    return { key, {} };
}

// One file imported as the type its extension names. The switch has no
// default, so a type added to ImportTypes without a case here is a compiler
// warning
[[nodiscard]] inline Imported_
importOne_(Vault* vault, const PathKey& dir, const Coco::Path& path)
{
    switch (ImportTypes::typeOf(path)) {
    case ImportTypes::Type::Copy:
        return importCopy_(vault, dir, path);
    case ImportTypes::Type::Docx:
        return importDocx_(vault, dir, path);
    case ImportTypes::Type::Unsupported:
        break;
    }

    return { {},
             QCoreApplication::translate(
                 "FileImport",
                 "Fernanda can't import \"%1\".")
                 .arg(path.nameQString()) };
}

} // namespace Internal

// Asks for files to import, over parent, starting in importFolder's folder,
// and imports each into dir as the type its extension names (ImportTypes): a
// converted type's text becomes a .txt file named for it, and a type Fernanda
// opens is copied in unchanged. One of neither type, chosen by typing its
// name, isn't touched. A file that fails leaves nothing in the vault, and
// every file that fails is listed in one warning, each with its reason. The
// folder the files were chosen from goes to importFolder, whether or not they
// import. Returns the keys of the files made, in the order chosen: empty when
// the dialog is cancelled or nothing imports. What to open or show afterward
// is the caller's
inline QList<PathKey> importFiles(
    QWidget* parent,
    Vault* vault,
    const PathKey& dir,
    const ImportFolder& importFolder)
{
    auto paths = Coco::getFiles(
        parent,
        QCoreApplication::translate("FileImport", "Import"),
        importFolder.get(),
        Internal::importFilter_());

    if (!paths.isEmpty()) {
        importFolder.set(paths.first().parent());
    }

    QList<PathKey> imported{};
    QStringList failures{};

    for (const auto& path : paths) {
        auto result = Internal::importOne_(vault, dir, path);

        if (result.key.isEmpty()) {
            failures << result.failure;
            continue;
        }

        imported << result.key;
    }

    if (!failures.isEmpty()) {
        QMessageBox::warning(
            parent,
            QCoreApplication::translate("FileImport", "Couldn't import"),
            QCoreApplication::translate(
                "FileImport",
                "%Ln file(s) couldn't be imported:",
                nullptr,
                int(failures.size())) +
                u"\n\n"_s + failures.join(u'\n'));
    }

    return imported;
}

} // namespace Fernanda::Ui
