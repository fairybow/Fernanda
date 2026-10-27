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

#include <QObject>
#include <QString>
#include <QTest>

#include "core/git/GitSafeDirectory.h"

using namespace Qt::StringLiterals;
using namespace Fernanda;

// Reading the folder Git refuses to trust from its message
// (core/git/GitSafeDirectory.h). The messages are Git's own, as each version
// writes them.
//
// Qt Test runs every private slot as a test. A slot named <test>_data fills a
// table, and <test> then runs once per row, reported under the row's name
class GitSafeDirectoryTest : public QObject
{
    Q_OBJECT

private slots:
    void folderOfReadsGitsMessage_data()
    {
        QTest::addColumn<QString>("error");
        QTest::addColumn<QString>("expected");

        QTest::newRow("unquoted")
            << u"fatal: detected dubious ownership in repository at "
               "'/home/a/novel'\nTo add an exception for this directory, "
               "call:\n\n\tgit config --global --add safe.directory "
               "/home/a/novel"_s
            << u"/home/a/novel"_s;

        QTest::newRow("quoted, with a space")
            << u"fatal: detected dubious ownership in repository at "
               "'/home/a/my novel'\nTo add an exception for this directory, "
               "call:\n\n\tgit config --global --add safe.directory "
               "'/home/a/my novel'"_s
            << u"/home/a/my novel"_s;

        QTest::newRow("quoted, with a quote")
            << u"fatal: detected dubious ownership\n\n\tgit config --global "
               "--add safe.directory '/home/a/Ann'\\''s novel'"_s
            << u"/home/a/Ann's novel"_s;

        QTest::newRow("a Windows drive")
            << u"fatal: detected dubious ownership in repository at "
               "'E:/Novel'\r\nTo add an exception for this directory, "
               "call:\r\n\r\n\tgit config --global --add safe.directory "
               "E:/Novel\r\n"_s
            << u"E:/Novel"_s;

        QTest::newRow("translated, with the command as it is")
            << u"fatal: unsichere Besitzverh\u00E4ltnisse im Repository "
               "'/home/a/novel' erkannt\n\n\tgit config --global --add "
               "safe.directory /home/a/novel"_s
            << u"/home/a/novel"_s;

        QTest::newRow("no repository")
            << u"fatal: not a git repository (or any of the parent "
               "directories): .git"_s
            << QString{};

        QTest::newRow("empty") << QString{} << QString{};
    }

    void folderOfReadsGitsMessage()
    {
        QFETCH(QString, error);
        QFETCH(QString, expected);

        QCOMPARE(GitSafeDirectory::folderOf(error), expected);
    }
};

QTEST_MAIN(GitSafeDirectoryTest)

#include "GitSafeDirectoryTest.moc"
