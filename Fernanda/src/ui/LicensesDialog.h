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

#pragma once

#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QIODevice>
#include <QLabel>
#include <QList>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QString>
#include <QStringList>
#include <QVBoxLayout>
#include <QWidget>
#include <QtGlobal>

#include <Coco/Debug.h>

#include "ui/UiConstants.h"

namespace Fernanda::Ui {

using namespace Qt::StringLiterals;

// The licenses of Fernanda and of everything it bundles, opened from the About
// panel. A list down the left, one item per work; for the one selected, what
// it is, its license and home page, and the license's full text.
//
// The texts are compiled in from resources/Licenses.qrc, under :/licenses/.
// Most are the files that came with each work, referred to where they sit in
// the repo; entries_ lists them. The text is shown in the system's fixed-pitch
// font, since license files are wrapped by hand to a width in characters.
//
// Window-modal and opened with open(), never exec() (see "Dialogs and event
// loops" in docs/Architecture.md). It deletes itself when closed
class LicensesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LicensesDialog(QWidget* parentWindow)
        : QDialog(parentWindow)
    {
        setup_();
    }

    ~LicensesDialog() override { TRACER; }

private:
    // One work: its name, what Fernanda uses it for (rich text, so it can
    // carry links), the license it is used under, its home page, and the
    // resources holding its license text. Several files are shown one after
    // another
    struct Entry_
    {
        QString name{};
        QString description{};
        QString license{};
        QString homePage{};
        QStringList files{};
    };

    QList<Entry_> entries_{};

    QListWidget* list_ = new QListWidget(this);
    QLabel* name_ = new QLabel(this);
    QLabel* description_ = new QLabel(this);
    QLabel* license_ = new QLabel(this);
    QLabel* homePage_ = new QLabel(this);
    QPlainTextEdit* text_ = new QPlainTextEdit(this);

    void setup_()
    {
        setWindowTitle(tr("Licenses"));
        setWindowModality(Qt::WindowModal);
        setAttribute(Qt::WA_DeleteOnClose);
        resize(LICENSES_DIALOG_WIDTH, LICENSES_DIALOG_HEIGHT);

        entries_ = makeEntries_();

        for (const auto& entry : entries_) {
            list_->addItem(entry.name);
        }

        list_->setFixedWidth(LICENSES_LIST_WIDTH);

        auto name_font = name_->font();
        name_font.setBold(true);
        name_->setFont(name_font);

        description_->setWordWrap(true);
        description_->setTextFormat(Qt::RichText);
        description_->setOpenExternalLinks(true);

        homePage_->setTextFormat(Qt::RichText);
        homePage_->setOpenExternalLinks(true);

        text_->setReadOnly(true);
        text_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

        auto* details = new QVBoxLayout;
        details->addWidget(name_);
        details->addWidget(description_);
        details->addWidget(license_);
        details->addWidget(homePage_);
        details->addWidget(text_, 1);

        auto* columns = new QHBoxLayout;
        columns->addWidget(list_);
        columns->addLayout(details, 1);

        auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
        connect(
            buttons,
            &QDialogButtonBox::rejected,
            this,
            &LicensesDialog::reject);

        auto* layout = new QVBoxLayout(this);
        layout->addLayout(columns, 1);
        layout->addWidget(buttons);

        connect(
            list_,
            &QListWidget::currentRowChanged,
            this,
            &LicensesDialog::showEntry_);

        list_->setCurrentRow(0);
    }

    void showEntry_(int row)
    {
        if (row < 0 || row >= entries_.size()) {
            return;
        }

        const auto& entry = entries_.at(row);

        name_->setText(entry.name);
        description_->setText(entry.description);
        license_->setText(tr("License: %1").arg(entry.license));
        homePage_->setText(
            u"<a href=\"%1\">%1</a>"_s.arg(entry.homePage.toHtmlEscaped()));

        QStringList texts{};

        for (const auto& path : entry.files) {
            texts << read_(path);
        }

        text_->setPlainText(texts.join(u"\n\n"_s));
    }

    // A license text from the resources. A file that is missing (left out
    // of the qrc) shows as a note, not as nothing
    [[nodiscard]] static QString read_(const QString& path)
    {
        QFile file(path);

        if (!file.open(QIODevice::ReadOnly)) {
            WARN("Can't read the license text {}", path);
            return tr("(The license text %1 is missing from this build.)")
                .arg(path);
        }

        return QString::fromUtf8(file.readAll());
    }

    // Fernanda first, then what it is built on, then what it bundles
    [[nodiscard]] static QList<Entry_> makeEntries_()
    {
        auto font = tr("A font bundled for the text.");
        auto ofl = u"SIL Open Font License 1.1"_s;

        QList<Entry_> entries{};

        entries.append(
            { u"Fernanda"_s,
              tr("This program, and Coco, its support library."),
              u"GNU GPL v3"_s,
              u"https://github.com/fairybow/Fernanda"_s,
              { u":/licenses/Fernanda"_s } });

        entries.append(
            { u"Qt"_s,
              tr("The application framework, version %1. Its source is "
                 "available from "
                 "<a href=\"https://download.qt.io/official_releases/qt/\">"
                 "download.qt.io</a>. Qt includes other libraries, listed "
                 "with their licenses at "
                 "<a href=\"https://doc.qt.io/qt-6/licenses-used-in-qt.html\">"
                 "doc.qt.io</a>. The LGPL is a set of extra permissions on "
                 "top of the GNU GPL v3, whose full text is under the Fernanda "
                 "entry.")
                  .arg(QString::fromLatin1(qVersion())),
              u"GNU LGPL v3"_s,
              u"https://www.qt.io"_s,
              { u":/licenses/Qt"_s } });

        // Hunspell's own notice, then MySpell's, which it is built on
        QStringList hunspell_files{ u":/licenses/Hunspell"_s,
                                    u":/licenses/MySpell"_s };

        entries.append(
            { u"Hunspell"_s,
              tr("Spellcheck. Used under the GNU GPL v2 or later, one of its "
                 "three licenses. It includes MySpell, whose notice "
                 "follows its own."),
              u"MPL 1.1, GNU GPL v2+, or GNU LGPL v2.1+"_s,
              u"https://hunspell.github.io"_s,
              hunspell_files });

        entries.append(
            { tr("English (US) dictionary"),
              tr("The bundled spelling dictionary, made from SCOWL by Kevin "
                 "Atkinson, as LibreOffice publishes it."),
              tr("SCOWL and its sources' terms"),
              u"http://wordlist.sourceforge.net"_s,
              { u":/licenses/en_US"_s } });

        entries.append(
            { u"Courier Prime"_s,
              font,
              ofl,
              u"https://quoteunquoteapps.com/courierprime/"_s,
              { u":/licenses/CourierPrime"_s } });

        entries.append(
            { u"Literata"_s,
              font,
              ofl,
              u"https://github.com/googlefonts/literata"_s,
              { u":/licenses/Literata"_s } });

        entries.append(
            { u"mononoki"_s,
              font,
              ofl,
              u"https://github.com/madmalik/mononoki"_s,
              { u":/licenses/Mononoki"_s } });

        entries.append(
            { u"OpenDyslexic"_s,
              font,
              ofl,
              u"https://opendyslexic.org"_s,
              { u":/licenses/OpenDyslexic"_s } });

        entries.append(
            { u"Lucide"_s,
              tr("The interface icons. Some derive from Feather, under the "
                 "MIT license, as the text says."),
              u"ISC"_s,
              u"https://lucide.dev"_s,
              { u":/licenses/Lucide"_s } });

        return entries;
    }
};

} // namespace Fernanda::Ui
