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

#include <QByteArray>
#include <QDir>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QTest>
#include <QtTypes>

#include <Coco/Path.h>

#include "core/files/Io.h"
#include "core/files/Zip.h"

using namespace Qt::StringLiterals;

// Zip archives (core/files/Zip.h): what's written reads back, names match
// exactly, no entry name can point outside an archive's folder, a failed write
// leaves the old archive alone, and an entry over the reader's limit isn't
// decompressed. Each test works in its own temporary folder.
//
// Qt Test runs every private slot as a test
class ZipTest : public QObject
{
    Q_OBJECT

private:
    // A limit no entry in these tests comes near
    static constexpr qint64 ANY_SIZE_ = 1024 * 1024;

    struct Fixture_
    {
        QTemporaryDir folder{};

        [[nodiscard]] Coco::Path absolute(const QString& relative) const
        {
            return Coco::Path(folder.filePath(relative));
        }

        void write(const QString& relative, const QByteArray& bytes) const
        {
            QVERIFY(
                Fernanda::Io::write(
                    bytes,
                    absolute(relative),
                    Fernanda::Io::CreateDirs::Yes));
        }

        [[nodiscard]] QByteArray read(const QString& relative) const
        {
            return Fernanda::Io::read(absolute(relative));
        }

        [[nodiscard]] Fernanda::Zip::Entry
        entry(const QString& relative, const QString& name) const
        {
            return { absolute(relative), name };
        }

        // The folder's top-level entries, sorted
        [[nodiscard]] QStringList names() const
        {
            auto names =
                QDir(folder.path())
                    .entryList(QDir::AllEntries | QDir::NoDotAndDotDot);
            names.sort();
            return names;
        }
    };

private slots:
    void writtenEntriesReadBack()
    {
        Fixture_ f{};
        auto accented = u"caf\u00E9"_s;
        f.write(u"one.txt"_s, "one");
        f.write(u"notes/"_s + accented + u".txt"_s, "\xC3\xA9t\xC3\xA9");

        auto archive = f.absolute(accented + u".zip"_s);

        QVERIFY(
            Fernanda::Zip::write(
                archive,
                { f.entry(u"one.txt"_s, u"one.txt"_s),
                  f.entry(
                      u"notes/"_s + accented + u".txt"_s,
                      u"notes/"_s + accented + u".txt"_s) }));

        QCOMPARE(
            Fernanda::Zip::readEntry(archive, u"one.txt"_s, ANY_SIZE_)
                .value_or(QByteArray{}),
            QByteArray("one"));
        QCOMPARE(
            Fernanda::Zip::readEntry(
                archive,
                u"notes/"_s + accented + u".txt"_s,
                ANY_SIZE_)
                .value_or(QByteArray{}),
            QByteArray("\xC3\xA9t\xC3\xA9"));
    }

    // An empty entry reads as empty bytes, not as a missing entry
    void anEmptyFileReadsBackEmpty()
    {
        Fixture_ f{};
        f.write(u"empty.txt"_s, {});

        auto archive = f.absolute(u"a.zip"_s);

        QVERIFY(
            Fernanda::Zip::write(
                archive,
                { f.entry(u"empty.txt"_s, u"empty.txt"_s) }));

        auto data =
            Fernanda::Zip::readEntry(archive, u"empty.txt"_s, ANY_SIZE_);

        QVERIFY(data.has_value());
        QVERIFY(data->isEmpty());
    }

    void aMissingEntryReadsAsNothing()
    {
        Fixture_ f{};
        f.write(u"a.txt"_s, "a");

        auto archive = f.absolute(u"a.zip"_s);

        QVERIFY(
            Fernanda::Zip::write(archive, { f.entry(u"a.txt"_s, u"a.txt"_s) }));

        QVERIFY(!Fernanda::Zip::readEntry(archive, u"b.txt"_s, ANY_SIZE_)
                     .has_value());
    }

    void entryNamesMatchLetterCase()
    {
        Fixture_ f{};
        f.write(u"a.txt"_s, "a");

        auto archive = f.absolute(u"a.zip"_s);

        QVERIFY(
            Fernanda::Zip::write(archive, { f.entry(u"a.txt"_s, u"a.txt"_s) }));

        QVERIFY(!Fernanda::Zip::readEntry(archive, u"A.txt"_s, ANY_SIZE_)
                     .has_value());
    }

    void entryNamesStayInsideTheFolder_data()
    {
        QTest::addColumn<QString>("name");
        QTest::addColumn<bool>("allowed");

        QTest::newRow("file") << u"a.txt"_s << true;
        QTest::newRow("file in a folder") << u"notes/a.txt"_s << true;
        QTest::newRow("colon inside a name") << u"Act 1: End.txt"_s << true;
        QTest::newRow("drive-like part after the first")
            << u"notes/C:x.txt"_s << true;
        QTest::newRow("digit before a colon") << u"1:x.txt"_s << true;

        QTest::newRow("empty") << QString{} << false;
        QTest::newRow("leading slash") << u"/a.txt"_s << false;
        QTest::newRow("backslash") << u"notes\\a.txt"_s << false;
        QTest::newRow("empty part") << u"notes//a.txt"_s << false;
        QTest::newRow("trailing slash") << u"notes/"_s << false;
        QTest::newRow("dot part") << u"./a.txt"_s << false;
        QTest::newRow("leading dot-dot") << u"../a.txt"_s << false;
        QTest::newRow("inner dot-dot") << u"notes/../a.txt"_s << false;
        QTest::newRow("drive-relative") << u"C:x.txt"_s << false;
        QTest::newRow("drive-absolute") << u"C:/x.txt"_s << false;
        QTest::newRow("lowercase drive") << u"c:x.txt"_s << false;
        QTest::newRow("bare drive") << u"C:"_s << false;
    }

    void entryNamesStayInsideTheFolder()
    {
        QFETCH(QString, name);
        QFETCH(bool, allowed);

        QCOMPARE(Fernanda::Zip::isEntryName(name), allowed);
    }

    void aFileThatIsntAZipReadsAsNothing()
    {
        Fixture_ f{};
        f.write(u"fake.zip"_s, "not a zip");

        QVERIFY(!Fernanda::Zip::readEntry(
                     f.absolute(u"fake.zip"_s),
                     u"a.txt"_s,
                     ANY_SIZE_)
                     .has_value());
    }

    // An entry whose source can't be read fails the whole write. The archive
    // already at the path is untouched, and no temporary file is left beside
    // it
    void aFailedWriteKeepsTheOldArchive()
    {
        Fixture_ f{};
        f.write(u"a.txt"_s, "first");

        auto archive = f.absolute(u"a.zip"_s);

        QVERIFY(
            Fernanda::Zip::write(archive, { f.entry(u"a.txt"_s, u"a.txt"_s) }));

        auto before = f.read(u"a.zip"_s);
        f.write(u"a.txt"_s, "second");

        QVERIFY(!Fernanda::Zip::write(
            archive,
            { f.entry(u"a.txt"_s, u"a.txt"_s),
              f.entry(u"missing.txt"_s, u"missing.txt"_s) }));

        QCOMPARE(f.read(u"a.zip"_s), before);
        QCOMPARE(
            Fernanda::Zip::readEntry(archive, u"a.txt"_s, ANY_SIZE_)
                .value_or(QByteArray{}),
            QByteArray("first"));
        QCOMPARE(f.names(), QStringList({ u"a.txt"_s, u"a.zip"_s }));
    }

    // The limit is checked against the size the archive declares: an entry of
    // exactly the limit reads, and one byte over doesn't
    void anEntryOverTheLimitIsRefused()
    {
        Fixture_ f{};
        f.write(u"ten.txt"_s, "0123456789");

        auto archive = f.absolute(u"a.zip"_s);

        QVERIFY(
            Fernanda::Zip::write(
                archive,
                { f.entry(u"ten.txt"_s, u"ten.txt"_s) }));

        QCOMPARE(
            Fernanda::Zip::readEntry(archive, u"ten.txt"_s, 10)
                .value_or(QByteArray{}),
            QByteArray("0123456789"));
        QVERIFY(
            !Fernanda::Zip::readEntry(archive, u"ten.txt"_s, 9).has_value());
    }
};

QTEST_GUILESS_MAIN(ZipTest)

#include "ZipTest.moc"
