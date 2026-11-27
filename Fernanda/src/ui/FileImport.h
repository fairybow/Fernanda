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

#include <QCoreApplication>
#include <QList>
#include <QMessageBox>
#include <QString>
#include <QStringList>
#include <QWidget>

#include <Coco/Path.h>

#include "core/files/Docx.h"
#include "core/files/ImportTypes.h"
#include "core/files/PathKey.h"
#include "core/vault/Vault.h"

namespace Fernanda::Ui {

using namespace Qt::StringLiterals;

namespace Internal {

// A file read for import: its text, or nullopt and the message saying why it
// couldn't be read
struct ImportedText_
{
    std::optional<QString> text{};
    QString failure{};
};

// The name a type goes by in the dialog's filter
[[nodiscard]] inline QString importTypeName_(ImportTypes::Type type)
{
    switch (type) {
    case ImportTypes::Type::Docx:
        return QCoreApplication::translate("FileImport", "Word documents");
    case ImportTypes::Type::Unsupported:
        break;
    }

    return {};
}

// Every importable file first, then each type on its own: "Importable files
// (*.docx);;Word documents (*.docx)"
[[nodiscard]] inline QString importFilter_()
{
    QStringList every_pattern{};
    QStringList filters{};

    for (auto type : ImportTypes::types()) {
        auto patterns = ImportTypes::patternsOf(type);
        every_pattern << patterns;
        filters << importTypeName_(type) + u" ("_s + patterns.join(u' ') + u')';
    }

    filters.prepend(
        QCoreApplication::translate("FileImport", "Importable files") +
        u" ("_s + every_pattern.join(u' ') + u')');

    return filters.join(u";;"_s);
}

[[nodiscard]] inline ImportedText_ readDocx_(const Coco::Path& path)
{
    auto conversion = Docx::toPlainText(path);
    auto name = path.nameQString();

    switch (conversion.failure) {
    case Docx::Failure::None:
        return { conversion.text, {} };

    case Docx::Failure::NotAWordDocument:
        return { std::nullopt,
                 QCoreApplication::translate(
                     "FileImport",
                     "\"%1\" isn't a Word document Fernanda can read.")
                     .arg(name) };

    case Docx::Failure::Damaged:
        return { std::nullopt,
                 QCoreApplication::translate("FileImport", "\"%1\" is damaged.")
                     .arg(name) };
    }

    return { std::nullopt, {} };
}

// The file read as the type its extension names. The switch has no default,
// so a type added to ImportTypes without a case here is a compiler warning
[[nodiscard]] inline ImportedText_ readForImport_(const Coco::Path& path)
{
    switch (ImportTypes::typeOf(path)) {
    case ImportTypes::Type::Docx:
        return readDocx_(path);
    case ImportTypes::Type::Unsupported:
        break;
    }

    return { std::nullopt,
             QCoreApplication::translate(
                 "FileImport",
                 "Fernanda can't import \"%1\".")
                 .arg(path.nameQString()) };
}

} // namespace Internal

// Asks for files to import, over parent, and imports each into dir as a text
// file named for it. Each file is read as the type its extension names
// (ImportTypes); one of no importable type, chosen by typing its name, isn't
// read at all. A file that fails leaves nothing in the vault, and every file
// that fails is listed in one warning, each with its reason. Returns the keys
// of the files made, in the order chosen: empty when the dialog is cancelled
// or nothing imports. What to open or show afterward is the caller's
inline QList<PathKey>
importFiles(QWidget* parent, Vault* vault, const PathKey& dir)
{
    auto paths = Coco::getFiles(
        parent,
        QCoreApplication::translate("FileImport", "Import"),
        Coco::Path{},
        Internal::importFilter_());

    QList<PathKey> imported{};
    QStringList failures{};

    for (const auto& path : paths) {
        auto read = Internal::readForImport_(path);

        if (!read.text) {
            failures << read.failure;
            continue;
        }

        auto key = vault->createFile(dir, path.stemQString(), *read.text);

        if (key.isEmpty()) {
            failures << QCoreApplication::translate(
                            "FileImport",
                            "\"%1\" couldn't be saved in the vault.")
                            .arg(path.nameQString());
            continue;
        }

        imported << key;
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
