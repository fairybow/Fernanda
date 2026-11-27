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

#include <QCoreApplication>
#include <QList>
#include <QMessageBox>
#include <QString>
#include <QStringList>
#include <QWidget>

#include <Coco/Path.h>

#include "core/files/Docx.h"
#include "core/files/PathKey.h"
#include "core/vault/Vault.h"

namespace Fernanda::Ui {

using namespace Qt::StringLiterals;

namespace Internal {

[[nodiscard]] inline QString
importFailure_(const Coco::Path& path, Docx::Failure failure)
{
    auto name = path.nameQString();

    if (failure == Docx::Failure::Damaged) {
        return QCoreApplication::translate("FileImport", "\"%1\" is damaged.")
            .arg(name);
    }

    return QCoreApplication::translate(
               "FileImport",
               "\"%1\" isn't a Word document Fernanda can read.")
        .arg(name);
}

} // namespace Internal

// Asks for files to import, over parent, and imports each into dir as a text
// file named for it. Word documents are the only kind for now. A file that
// fails leaves nothing in the vault, and every file that fails is listed in
// one warning, each with its reason. Returns the keys of the files made, in
// the order chosen: empty when the dialog is cancelled or nothing imports.
// What to open or show afterward is the caller's
inline QList<PathKey>
importFiles(QWidget* parent, Vault* vault, const PathKey& dir)
{
    auto paths = Coco::getFiles(
        parent,
        QCoreApplication::translate("FileImport", "Import"),
        Coco::Path{},
        QCoreApplication::translate("FileImport", "Word documents (*.docx)"));

    QList<PathKey> imported{};
    QStringList failures{};

    for (const auto& path : paths) {
        auto conversion = Docx::toPlainText(path);

        if (conversion.failure != Docx::Failure::None) {
            failures << Internal::importFailure_(path, conversion.failure);
            continue;
        }

        auto key = vault->createFile(dir, path.stemQString(), conversion.text);

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
