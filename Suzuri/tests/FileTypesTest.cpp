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

#include <QMetaType>
#include <QObject>
#include <QString>
#include <QTest>

#include <Coco/Path.h>

#include "core/FileTypes.h"

using namespace Qt::StringLiterals;
using Suzuri::FileTypes::Type;

// Lets a Type sit in a test table's column
Q_DECLARE_METATYPE(Suzuri::FileTypes::Type)

// Which files Suzuri opens, and as what (core/FileTypes.h).
//
// Qt Test runs every private slot as a test. A slot named <test>_data fills a
// table, and <test> then runs once per row, reported under the row's name
class FileTypesTest : public QObject
{
    Q_OBJECT

private slots:
    void typeOf_data()
    {
        QTest::addColumn<QString>("path");
        QTest::addColumn<Type>("expected");

        QTest::newRow("txt") << u"a.txt"_s << Type::Text;
        QTest::newRow("md") << u"a.md"_s << Type::Text;
        QTest::newRow("markdown") << u"a.markdown"_s << Type::Text;
        QTest::newRow("fountain") << u"a.fountain"_s << Type::Text;
        QTest::newRow("pdf") << u"a.pdf"_s << Type::Pdf;
        QTest::newRow("png") << u"a.png"_s << Type::Image;
        QTest::newRow("jpg") << u"a.jpg"_s << Type::Image;
        QTest::newRow("jpeg") << u"a.jpeg"_s << Type::Image;
        QTest::newRow("gif") << u"a.gif"_s << Type::Image;
        QTest::newRow("bmp") << u"a.bmp"_s << Type::Image;
        QTest::newRow("tif") << u"a.tif"_s << Type::Image;
        QTest::newRow("tiff") << u"a.tiff"_s << Type::Image;
        QTest::newRow("webp") << u"a.webp"_s << Type::Image;

        // Letter case doesn't matter
        QTest::newRow("upper case") << u"a.TXT"_s << Type::Text;
        QTest::newRow("mixed case") << u"a.Md"_s << Type::Text;
        QTest::newRow("upper case image") << u"a.PNG"_s << Type::Image;

        // Only the last extension counts, and only the name is read
        QTest::newRow("two extensions") << u"a.tar.md"_s << Type::Text;
        QTest::newRow("in folders") << u"one/two/a.pdf"_s << Type::Pdf;
        QTest::newRow("dotted folder") << u"v1.0/a.txt"_s << Type::Text;
        QTest::newRow("name outside ASCII") << u"\u00E9.txt"_s << Type::Text;

        // Anything not in the table is unsupported: text is never the
        // fallback
        QTest::newRow("docx") << u"a.docx"_s << Type::Unsupported;
        QTest::newRow("svg") << u"a.svg"_s << Type::Unsupported;
        QTest::newRow("no extension") << u"Makefile"_s << Type::Unsupported;
        QTest::newRow("empty") << u""_s << Type::Unsupported;
        QTest::newRow("backup of a text file")
            << u"a.txt.bak"_s << Type::Unsupported;
        QTest::newRow("trailing period") << u"a."_s << Type::Unsupported;

        // A leading period starts a name, not an extension
        QTest::newRow("dotfile named like an extension")
            << u".txt"_s << Type::Unsupported;

        // A period inside a name starts an extension, wanted or not
        QTest::newRow("sentence-like name")
            << u"Chapter 1. The Start"_s << Type::Unsupported;
    }

    void typeOf()
    {
        QFETCH(QString, path);
        QFETCH(Type, expected);

        QCOMPARE(Suzuri::FileTypes::typeOf(Coco::Path(path)), expected);
    }

    // isSupported is "typeOf isn't Unsupported", for every row of typeOf's
    // table
    void isSupported_data() { typeOf_data(); }

    void isSupported()
    {
        QFETCH(QString, path);
        QFETCH(Type, expected);

        QCOMPARE(
            Suzuri::FileTypes::isSupported(Coco::Path(path)),
            expected != Type::Unsupported);
    }

    void isHiddenName_data()
    {
        QTest::addColumn<QString>("name");
        QTest::addColumn<bool>("expected");

        QTest::newRow("vault folder") << u".suzuri"_s << true;
        QTest::newRow("git folder") << u".git"_s << true;
        QTest::newRow("dotfile") << u".gitignore"_s << true;
        QTest::newRow("file") << u"a.txt"_s << false;
        QTest::newRow("folder") << u"Drafts"_s << false;
        QTest::newRow("period inside") << u"v1.0"_s << false;
        QTest::newRow("empty") << u""_s << false;
    }

    void isHiddenName()
    {
        QFETCH(QString, name);
        QFETCH(bool, expected);

        QCOMPARE(Suzuri::FileTypes::isHiddenName(name), expected);
    }
};

QTEST_GUILESS_MAIN(FileTypesTest)

#include "FileTypesTest.moc"
