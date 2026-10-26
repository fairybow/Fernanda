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
#include <QList>
#include <QObject>
#include <QString>
#include <QTest>
#include <QtTypes>

#include "core/git/GitDiff.h"

using namespace Qt::StringLiterals;
using namespace Fernanda;

// Reading what `git diff` prints (core/git/GitDiff.h). Each case is a diff in
// git's unified format, written out whole, or, for the words that changed in
// a line, the line before and after (GitDiff::wordChanges).
//
// Qt Test runs every private slot as a test
class GitDiffTest : public QObject
{
    Q_OBJECT

private:
    using Kind_ = GitDiff::Line::Kind;
    using Span_ = GitDiff::Span;

    // A line's changes, as one span
    [[nodiscard]] static QList<Span_>
    onlySpan_(qsizetype start, qsizetype length)
    {
        return {
            Span_{ start, length }
        };
    }

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

    // Git's note follows the line it's about, has no numbers, and doesn't
    // keep the lines around it from pairing
    void noNewlineNoteIsALine()
    {
        auto diff = GitDiff::parse(
            "@@ -1 +1 @@\n"
            "-old line\n"
            "\\ No newline at end of file\n"
            "+new line\n"
            "\\ No newline at end of file\n");

        const auto& lines = diff.hunks[0].lines;
        QCOMPARE(lines.size(), 4);
        QCOMPARE(lines[1].kind, Kind_::NoNewline);
        QCOMPARE(lines[1].text, u"No newline at end of file"_s);
        QCOMPARE(lines[1].oldNumber, 0);
        QCOMPARE(lines[1].newNumber, 0);
        QCOMPARE(lines[2].kind, Kind_::Added);
        QCOMPARE(lines[2].newNumber, 1);
        QCOMPARE(lines[3].kind, Kind_::NoNewline);

        QCOMPARE(lines[0].changes, onlySpan_(0, 3));
        QCOMPARE(lines[2].changes, onlySpan_(0, 3));
    }

    // A file given a line break at its end: the line is the same either side,
    // so nothing in it is marked, and the note says why it's there
    void addedFinalLineBreakMarksNoWords()
    {
        auto diff = GitDiff::parse(
            "@@ -1 +1 @@\n"
            "-last line\n"
            "\\ No newline at end of file\n"
            "+last line\n");

        const auto& lines = diff.hunks[0].lines;
        QCOMPARE(lines.size(), 3);
        QVERIFY(lines[0].changes.isEmpty());
        QVERIFY(lines[2].changes.isEmpty());
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
    void changedWordIsMarkedOnEachSide()
    {
        auto changes = GitDiff::wordChanges(
            u"The cat sat down."_s,
            u"The dog sat down."_s);

        QCOMPARE(changes.before, onlySpan_(4, 3));
        QCOMPARE(changes.after, onlySpan_(4, 3));
    }

    // In a paragraph, two edits far apart are two spans, not one from the
    // first to the last
    void editsFarApartAreSeparateSpans()
    {
        auto before = u"She walked to the old house at the end of the lane, "
                      u"where the lamps were never lit."_s;
        auto after = u"She ran to the old house at the end of the lane, "
                     u"where the lamps were always lit."_s;

        auto changes = GitDiff::wordChanges(before, after);

        QCOMPARE(changes.before.size(), 2);
        QCOMPARE(
            before.sliced(changes.before[0].start, changes.before[0].length),
            u"walked"_s);
        QCOMPARE(
            before.sliced(changes.before[1].start, changes.before[1].length),
            u"never"_s);

        QCOMPARE(changes.after.size(), 2);
        QCOMPARE(
            after.sliced(changes.after[0].start, changes.after[0].length),
            u"ran"_s);
        QCOMPARE(
            after.sliced(changes.after[1].start, changes.after[1].length),
            u"always"_s);
    }

    // The space between two changed words is marked with them
    void rewordedPhraseIsOneSpan()
    {
        auto before = u"I went to the store today"_s;
        auto after = u"I drove past the store today"_s;

        auto changes = GitDiff::wordChanges(before, after);

        QCOMPARE(changes.before, onlySpan_(2, 7));
        QCOMPARE(changes.after, onlySpan_(2, 10));
    }

    void addedWordIsMarkedOnlyAfter()
    {
        auto changes = GitDiff::wordChanges(
            u"It was a cold night."_s,
            u"It was a very cold night."_s);

        QVERIFY(changes.before.isEmpty());
        QCOMPARE(changes.after, onlySpan_(9, 5));
    }

    void changedSpacingIsMarked()
    {
        auto changes =
            GitDiff::wordChanges(u"one  two three"_s, u"one two three"_s);

        QCOMPARE(changes.before, onlySpan_(3, 2));
        QCOMPARE(changes.after, onlySpan_(3, 1));
    }

    void sameLineMarksNothing()
    {
        auto changes = GitDiff::wordChanges(u"Same."_s, u"Same."_s);

        QVERIFY(changes.before.isEmpty());
        QVERIFY(changes.after.isEmpty());
    }

    // Only "I", "a", and the period are left of the old line: fewer than half
    // its characters
    void rewrittenLineMarksNothing()
    {
        auto changes = GitDiff::wordChanges(
            u"I like a quiet cat."_s,
            u"I bought a loud parrot."_s);

        QVERIFY(changes.before.isEmpty());
        QVERIFY(changes.after.isEmpty());
    }

    // An accent, precomposed or combining, is part of its word
    void accentedWordIsOneWord()
    {
        auto precomposed =
            GitDiff::wordChanges(u"Cafe au lait"_s, u"Caf\u00E9 au lait"_s);
        QCOMPARE(precomposed.before, onlySpan_(0, 4));
        QCOMPARE(precomposed.after, onlySpan_(0, 4));

        auto combining =
            GitDiff::wordChanges(u"Cafe au lait"_s, u"Cafe\u0301 au lait"_s);
        QCOMPARE(combining.before, onlySpan_(0, 4));
        QCOMPARE(combining.after, onlySpan_(0, 5));
    }

    // A character outside the Basic Multilingual Plane is two QChars, and
    // one character
    void surrogatePairIsOneCharacter()
    {
        auto changes = GitDiff::wordChanges(
            u"one two \U0001F600 three"_s,
            u"one two \U0001F603 three"_s);

        QCOMPARE(changes.before, onlySpan_(8, 2));
        QCOMPARE(changes.after, onlySpan_(8, 2));
    }

    // Han characters are words by themselves, so one changed character is
    // marked alone
    void hanCharacterIsAWord()
    {
        auto changes = GitDiff::wordChanges(
            u"\u6211\u559C\u6B22\u732B\u548C\u72D7"_s,
            u"\u6211\u559C\u6B22\u732B\u548C\u9E1F"_s);

        QCOMPARE(changes.before, onlySpan_(5, 1));
        QCOMPARE(changes.after, onlySpan_(5, 1));
    }

    // Middles too large to compare word by word are marked whole, from the
    // first word that differs to the last, though they share words ("the")
    // a comparison would have found
    void largeMiddleIsOneSpan()
    {
        auto common = QString(u"x "_s).repeated(10000);
        QString before_middle{};
        QString after_middle{};

        for (auto i = 0; i < 1200; ++i) {
            before_middle += u"a%1 the "_s.arg(i);
            after_middle += u"b%1 the "_s.arg(i);
        }

        auto before = common + before_middle + common;
        auto after = common + after_middle + common;

        auto changes = GitDiff::wordChanges(before, after);

        // The last " the " matches, and is left out
        auto shared_end = u" the "_s.size();
        QCOMPARE(changes.before.size(), 1);
        QCOMPARE(changes.before[0].start, common.size());
        QCOMPARE(changes.before[0].length, before_middle.size() - shared_end);
        QCOMPARE(changes.after.size(), 1);
        QCOMPARE(changes.after[0].length, after_middle.size() - shared_end);
    }

    void pairedLinesAreMarkedInOrder()
    {
        auto diff = GitDiff::parse(
            "@@ -1,3 +1,3 @@\n"
            "-first old line\n"
            "-second old line\n"
            "+first new line\n"
            "+second new line\n"
            " after\n");

        const auto& lines = diff.hunks[0].lines;
        QCOMPARE(lines[0].changes, onlySpan_(6, 3));
        QCOMPARE(lines[1].changes, onlySpan_(7, 3));
        QCOMPARE(lines[2].changes, onlySpan_(6, 3));
        QCOMPARE(lines[3].changes, onlySpan_(7, 3));
        QVERIFY(lines[4].changes.isEmpty());
    }

    // One line removed and two added: no telling which became which
    void unevenRunMarksNothing()
    {
        auto diff = GitDiff::parse(
            "@@ -1 +1,2 @@\n"
            "-one long line\n"
            "+one long\n"
            "+line\n");

        for (const auto& line : diff.hunks[0].lines) {
            QVERIFY(line.changes.isEmpty());
        }
    }

    // A context line ends a run: each side of it pairs on its own
    void contextLineEndsARun()
    {
        auto diff = GitDiff::parse(
            "@@ -1,3 +1,4 @@\n"
            "-red apple\n"
            "+green apple\n"
            " middle\n"
            "+brand new line\n");

        const auto& lines = diff.hunks[0].lines;
        QCOMPARE(lines[0].changes, onlySpan_(0, 3));
        QCOMPARE(lines[1].changes, onlySpan_(0, 5));
        QVERIFY(lines[3].changes.isEmpty());
    }
};

QTEST_GUILESS_MAIN(GitDiffTest)
#include "GitDiffTest.moc"
