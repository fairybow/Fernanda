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

#include <QByteArray>
#include <QDir>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QTest>

#include <Coco/Path.h>

#include "core/Io.h"

using namespace Qt::StringLiterals;

// Renaming files and folders on disk (core/Io.h): a plain rename, a refusal
// when the name is taken, and a change of letter case alone, which goes
// through a temporary name. Each test works in its own temporary folder.
//
// Qt Test runs every private slot as a test
class IoTest : public QObject
{
    Q_OBJECT

private:
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

        // The folder's entries as it lists them, so letter case shows
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
    void renameMovesAFileAndAFolder()
    {
        Fixture_ f{};
        f.write(u"a.txt"_s, "a");
        f.write(u"one/b.txt"_s, "b");

        QVERIFY(
            Fernanda::Io::rename(
                f.absolute(u"a.txt"_s),
                f.absolute(u"c.txt"_s)));
        QVERIFY(
            Fernanda::Io::rename(f.absolute(u"one"_s), f.absolute(u"two"_s)));

        QCOMPARE(f.names(), QStringList({ u"c.txt"_s, u"two"_s }));
        QCOMPARE(f.read(u"c.txt"_s), QByteArray("a"));
        QCOMPARE(f.read(u"two/b.txt"_s), QByteArray("b"));
    }

    // Nothing is replaced: both entries stay as they were
    void renameRefusesATakenName()
    {
        Fixture_ f{};
        f.write(u"a.txt"_s, "a");
        f.write(u"b.txt"_s, "b");
        f.write(u"one/x.txt"_s, "x");
        f.write(u"two/y.txt"_s, "y");

        QVERIFY(!Fernanda::Io::rename(
            f.absolute(u"a.txt"_s),
            f.absolute(u"b.txt"_s)));
        QVERIFY(
            !Fernanda::Io::rename(f.absolute(u"one"_s), f.absolute(u"two"_s)));

        QCOMPARE(
            f.names(),
            QStringList({ u"a.txt"_s, u"b.txt"_s, u"one"_s, u"two"_s }));
        QCOMPARE(f.read(u"a.txt"_s), QByteArray("a"));
        QCOMPARE(f.read(u"b.txt"_s), QByteArray("b"));
    }

    // Wherever letter case is ignored or not, the entry takes the new case,
    // and the temporary name is gone
    void aChangeOfCaseAloneTakesTheNewCase()
    {
        Fixture_ f{};
        f.write(u"notes.txt"_s, "n");
        f.write(u"box/a.txt"_s, "a");

        QVERIFY(
            Fernanda::Io::rename(
                f.absolute(u"notes.txt"_s),
                f.absolute(u"Notes.txt"_s)));
        QVERIFY(
            Fernanda::Io::rename(f.absolute(u"box"_s), f.absolute(u"BOX"_s)));

        QCOMPARE(f.names(), QStringList({ u"BOX"_s, u"Notes.txt"_s }));
        QCOMPARE(f.read(u"Notes.txt"_s), QByteArray("n"));
        QCOMPARE(f.read(u"BOX/a.txt"_s), QByteArray("a"));
    }

    // Where letter case counts, "notes" and "Notes" are two entries, and
    // changing one's case onto the other is refused like any taken name. The
    // entry goes back from its temporary name
    void aChangeOfCaseOntoAnotherEntryIsRefused()
    {
        Fixture_ f{};
        f.write(u"notes.txt"_s, "lower");
        f.write(u"Notes.txt"_s, "upper");

        if (f.names().size() != 2) {
            QSKIP("This file system ignores letter case");
        }

        QVERIFY(!Fernanda::Io::rename(
            f.absolute(u"notes.txt"_s),
            f.absolute(u"Notes.txt"_s)));

        QCOMPARE(f.names(), QStringList({ u"Notes.txt"_s, u"notes.txt"_s }));
        QCOMPARE(f.read(u"notes.txt"_s), QByteArray("lower"));
        QCOMPARE(f.read(u"Notes.txt"_s), QByteArray("upper"));
    }
};

QTEST_GUILESS_MAIN(IoTest)

#include "IoTest.moc"
