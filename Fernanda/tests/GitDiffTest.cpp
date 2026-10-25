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

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QTest>

#include "core/git/GitDiff.h"

using namespace Qt::StringLiterals;
using namespace Fernanda;

// Reading what `git diff` prints (core/git/GitDiff.h). Each case is a diff in
// git's unified format, written out whole.
//
// Qt Test runs every private slot as a test
class GitDiffTest : public QObject
{
    Q_OBJECT

private:
    using Kind_ = GitDiff::Line::Kind;

private slots:
    void emptyOutputHasNoHunks()
    {
        auto diff = GitDiff::parse({});

        QVERIFY(!diff.binary);
        QVERIFY(diff.hunks.isEmpty());
    }

    void modifiedFileNumbersEachLine()
    {
        auto diff = GitDiff::parse(
            "diff --git a/a.txt b/a.txt\n"
            "index 1111111..2222222 100644\n"
            "--- a/a.txt\n"
            "+++ b/a.txt\n"
            "@@ -1,3 +1,3 @@\n"
            " one\n"
            "-two\n"
            "+Two\n"
            " three\n");

        QCOMPARE(diff.hunks.size(), 1);

        const auto& hunk = diff.hunks[0];
        QCOMPARE(hunk.oldStart, 1);
        QCOMPARE(hunk.newStart, 1);
        QCOMPARE(hunk.header, u"@@ -1,3 +1,3 @@"_s);
        QCOMPARE(hunk.lines.size(), 4);

        QCOMPARE(hunk.lines[0].kind, Kind_::Context);
        QCOMPARE(hunk.lines[0].text, u"one"_s);
        QCOMPARE(hunk.lines[0].oldNumber, 1);
        QCOMPARE(hunk.lines[0].newNumber, 1);

        QCOMPARE(hunk.lines[1].kind, Kind_::Removed);
        QCOMPARE(hunk.lines[1].text, u"two"_s);
        QCOMPARE(hunk.lines[1].oldNumber, 2);
        QCOMPARE(hunk.lines[1].newNumber, 0);

        QCOMPARE(hunk.lines[2].kind, Kind_::Added);
        QCOMPARE(hunk.lines[2].text, u"Two"_s);
        QCOMPARE(hunk.lines[2].oldNumber, 0);
        QCOMPARE(hunk.lines[2].newNumber, 2);

        QCOMPARE(hunk.lines[3].kind, Kind_::Context);
        QCOMPARE(hunk.lines[3].oldNumber, 3);
        QCOMPARE(hunk.lines[3].newNumber, 3);
    }

    // Each hunk numbers from its own start, and the old and new numbers
    // drift apart as lines are added
    void secondHunkNumbersFromItsOwnStart()
    {
        auto diff = GitDiff::parse(
            "@@ -1,2 +1,3 @@\n"
            " a\n"
            "+b\n"
            " c\n"
            "@@ -40,2 +41,2 @@ Chapter Two\n"
            "-x\n"
            "+y\n"
            " z\n");

        QCOMPARE(diff.hunks.size(), 2);

        const auto& second = diff.hunks[1];
        QCOMPARE(second.oldStart, 40);
        QCOMPARE(second.newStart, 41);
        QCOMPARE(second.header, u"@@ -40,2 +41,2 @@ Chapter Two"_s);
        QCOMPARE(second.lines[0].oldNumber, 40);
        QCOMPARE(second.lines[1].newNumber, 41);
        QCOMPARE(second.lines[2].oldNumber, 41);
        QCOMPARE(second.lines[2].newNumber, 42);
    }

    // A hunk of one line leaves out its count: "-5 +5"
    void rangeWithoutACountIsRead()
    {
        auto diff = GitDiff::parse(
            "@@ -5 +5 @@\n"
            "-old\n"
            "+new\n");

        QCOMPARE(diff.hunks.size(), 1);
        QCOMPARE(diff.hunks[0].oldStart, 5);
        QCOMPARE(diff.hunks[0].lines[1].newNumber, 5);
    }

    void addedFileIsAllAdded()
    {
        auto diff = GitDiff::parse(
            "diff --git a/new.md b/new.md\n"
            "new file mode 100644\n"
            "--- /dev/null\n"
            "+++ b/new.md\n"
            "@@ -0,0 +1,2 @@\n"
            "+first\n"
            "+second\n");

        QCOMPARE(diff.hunks.size(), 1);
        QCOMPARE(diff.hunks[0].lines.size(), 2);
        QCOMPARE(diff.hunks[0].lines[0].kind, Kind_::Added);
        QCOMPARE(diff.hunks[0].lines[0].newNumber, 1);
        QCOMPARE(diff.hunks[0].lines[1].newNumber, 2);
    }

    // The empty line is a context line whose text is empty: " " in the diff
    void emptyContextLineIsKept()
    {
        auto diff = GitDiff::parse(
            "@@ -1,3 +1,3 @@\n"
            " a\n"
            " \n"
            "-b\n"
            "+B\n");

        QCOMPARE(diff.hunks[0].lines.size(), 4);
        QCOMPARE(diff.hunks[0].lines[1].kind, Kind_::Context);
        QVERIFY(diff.hunks[0].lines[1].text.isEmpty());
    }

    // A line removed or added that itself starts with "--" or "++" is still
    // a line, not a file name
    void lineLikeAFileNameIsALine()
    {
        auto diff = GitDiff::parse(
            "@@ -1,1 +1,1 @@\n"
            "--- a dash line\n"
            "+++ a plus line\n");

        QCOMPARE(diff.hunks[0].lines.size(), 2);
        QCOMPARE(diff.hunks[0].lines[0].text, u"-- a dash line"_s);
        QCOMPARE(diff.hunks[0].lines[1].text, u"++ a plus line"_s);
    }

    void noNewlineMarkerIsSkipped()
    {
        auto diff = GitDiff::parse(
            "@@ -1 +1 @@\n"
            "-old\n"
            "\\ No newline at end of file\n"
            "+new\n"
            "\\ No newline at end of file\n");

        QCOMPARE(diff.hunks[0].lines.size(), 2);
        QCOMPARE(diff.hunks[0].lines[1].kind, Kind_::Added);
    }

    void windowsLineEndingIsDropped()
    {
        auto diff = GitDiff::parse(
            "@@ -1 +1 @@\r\n"
            "-old\r\n"
            "+new\r\n");

        QCOMPARE(diff.hunks.size(), 1);
        QCOMPARE(diff.hunks[0].lines[0].text, u"old"_s);
        QCOMPARE(diff.hunks[0].lines[1].text, u"new"_s);
    }

    // "Caf\xC3\xA9" is UTF-8 for a word with an e-acute
    void textIsReadAsUtf8()
    {
        auto diff = GitDiff::parse(
            "@@ -1 +1 @@\n"
            "-Cafe\n"
            "+Caf\xC3\xA9\n");

        QCOMPARE(diff.hunks[0].lines[1].text, u"Café"_s);
    }

    void binaryFileHasNoHunks()
    {
        auto diff = GitDiff::parse(
            "diff --git a/map.png b/map.png\n"
            "index 1111111..2222222 100644\n"
            "Binary files a/map.png and b/map.png differ\n");

        QVERIFY(diff.binary);
        QVERIFY(diff.hunks.isEmpty());
    }

    // A text line in a hunk reading "Binary files ..." is text
    void binaryWordsInAHunkAreText()
    {
        auto diff = GitDiff::parse(
            "@@ -1 +1 @@\n"
            "-a\n"
            "+Binary files are not text\n");

        QVERIFY(!diff.binary);
        QCOMPARE(diff.hunks[0].lines[1].text, u"Binary files are not text"_s);
    }

    void modeChangeOnlyHasNoHunks()
    {
        auto diff = GitDiff::parse(
            "diff --git a/run.sh b/run.sh\n"
            "old mode 100644\n"
            "new mode 100755\n");

        QVERIFY(!diff.binary);
        QVERIFY(diff.hunks.isEmpty());
    }
};

QTEST_GUILESS_MAIN(GitDiffTest)
#include "GitDiffTest.moc"
