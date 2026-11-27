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

#include <QList>
#include <QMetaType>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTest>

#include <Coco/Path.h>

#include "core/files/ImportTypes.h"

using namespace Qt::StringLiterals;
using Fernanda::ImportTypes::Type;

// Lets a Type sit in a test table's column
Q_DECLARE_METATYPE(Fernanda::ImportTypes::Type)

// Which files Fernanda imports, and as what (core/files/ImportTypes.h).
//
// Qt Test runs every private slot as a test. A slot named <test>_data fills a
// table, and <test> then runs once per row, reported under the row's name
class ImportTypesTest : public QObject
{
    Q_OBJECT

private slots:
    void typeOf_data()
    {
        QTest::addColumn<QString>("path");
        QTest::addColumn<Type>("expected");

        QTest::newRow("docx") << u"a.docx"_s << Type::Docx;
        QTest::newRow("in folders") << u"one/two/a.docx"_s << Type::Docx;
        QTest::newRow("name with spaces")
            << u"Chapter 1 draft.docx"_s << Type::Docx;

        // Letter case doesn't matter
        QTest::newRow("upper case") << u"a.DOCX"_s << Type::Docx;
        QTest::newRow("mixed case") << u"a.Docx"_s << Type::Docx;

        // Anything not in the table is unsupported, files Fernanda opens
        // included: bringing those in would be a copy, not an import
        QTest::newRow("old Word") << u"a.doc"_s << Type::Unsupported;
        QTest::newRow("OpenDocument") << u"a.odt"_s << Type::Unsupported;
        QTest::newRow("txt") << u"a.txt"_s << Type::Unsupported;
        QTest::newRow("md") << u"a.md"_s << Type::Unsupported;
        QTest::newRow("backup of a document")
            << u"a.docx.bak"_s << Type::Unsupported;
        QTest::newRow("no extension") << u"docx"_s << Type::Unsupported;
        QTest::newRow("empty") << u""_s << Type::Unsupported;

        // A leading period starts a name, not an extension
        QTest::newRow("dotfile named like an extension")
            << u".docx"_s << Type::Unsupported;
    }

    void typeOf()
    {
        QFETCH(QString, path);
        QFETCH(Type, expected);

        QCOMPARE(Fernanda::ImportTypes::typeOf(Coco::Path(path)), expected);
    }

    void typesListsEachImportableTypeOnce()
    {
        QCOMPARE(Fernanda::ImportTypes::types(), QList<Type>{ Type::Docx });
    }

    void patternsOf()
    {
        QCOMPARE(
            Fernanda::ImportTypes::patternsOf(Type::Docx),
            QStringList{ u"*.docx"_s });
        QVERIFY(Fernanda::ImportTypes::patternsOf(Type::Unsupported).isEmpty());
    }

    // Every pattern names its own type, so the dialog's filter and typeOf
    // agree on what each file is
    void everyPatternIsItsOwnType()
    {
        for (auto type : Fernanda::ImportTypes::types()) {
            for (const auto& pattern :
                 Fernanda::ImportTypes::patternsOf(type)) {
                auto name = u"a"_s + pattern.mid(1);
                QCOMPARE(Fernanda::ImportTypes::typeOf(Coco::Path(name)), type);
            }
        }
    }
};

QTEST_GUILESS_MAIN(ImportTypesTest)

#include "ImportTypesTest.moc"
