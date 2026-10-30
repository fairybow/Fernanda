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

#include <QChar>
#include <QList>
#include <QObject>
#include <QPlainTextDocumentLayout>
#include <QString>
#include <QTest>
#include <QTextCursor>
#include <QTextDocument>

#include "models/PrimeDocument.h"
#include "views/TextSearch.h"

using namespace Qt::StringLiterals;
using Fernanda::PrimeDocument;
using Fernanda::TextSearch::Match;
using Fernanda::TextSearch::Options;

namespace TextSearch = Fernanda::TextSearch;

// What counts as a match, and what replacing does to a document
// (views/TextSearch.h). The last tests replace through a view's document on a
// prime, as the app does, to check that the buffer and the other views follow
// and that one undo takes a whole replace-all back.
//
// Text outside ASCII is built from code point numbers, so nothing here
// depends on how the compiler reads this file.
//
// Qt Test runs every private slot as a test
class TextSearchTest : public QObject
{
    Q_OBJECT

private:
    static constexpr Options PLAIN_{};
    static constexpr Options MATCH_CASE_{ true, false };
    static constexpr Options WHOLE_WORD_{ false, true };

    static void setUp_(QTextDocument& document, const QString& text)
    {
        document.setDocumentLayout(new QPlainTextDocumentLayout(&document));
        document.setPlainText(text);
        document.setModified(false);
    }

    // A document's text with nothing substituted, line breaks as '\n'
    [[nodiscard]] static QString textOf_(const QTextDocument& document)
    {
        auto text = document.toRawText();
        text.replace(QChar::ParagraphSeparator, QChar(u'\n'));
        return text;
    }

    // Where each match starts
    [[nodiscard]] static QList<int> positionsOf_(const QList<Match>& matches)
    {
        QList<int> positions{};

        for (const auto& match : matches) {
            positions << match.position;
        }

        return positions;
    }

    [[nodiscard]] static QList<int>
    positionsIn_(const QString& text, const QString& term, Options options)
    {
        QTextDocument document{};
        setUp_(document, text);

        return positionsOf_(TextSearch::findAll(&document, term, options));
    }

    [[nodiscard]] static QString noBreakSpace_() { return QChar(0x00A0); }

    // Outside the BMP, so two UTF-16 code units and two cursor positions
    [[nodiscard]] static QString emoji_()
    {
        return QString::fromUcs4(U"\x1F4C1", 1);
    }

private slots:
    // --- Finding ------------------------------------------------------------

    void findAllGivesEveryMatchInOrder()
    {
        QTextDocument document{};
        setUp_(document, u"one two one\nthree one"_s);

        auto matches = TextSearch::findAll(&document, u"one"_s, PLAIN_);

        QCOMPARE(positionsOf_(matches), (QList<int>{ 0, 8, 18 }));

        for (const auto& match : matches) {
            QCOMPARE(match.length, 3);
        }
    }

    void anEmptyTermMatchesNothing()
    {
        QVERIFY(positionsIn_(u"one two"_s, QString{}, PLAIN_).isEmpty());
    }

    void aTermThatIsNotThereMatchesNothing()
    {
        QVERIFY(positionsIn_(u"one two"_s, u"three"_s, PLAIN_).isEmpty());
        QVERIFY(positionsIn_(QString{}, u"one"_s, PLAIN_).isEmpty());
    }

    void letterCaseIsIgnoredUnlessAsked()
    {
        auto text = u"The the THE"_s;

        QCOMPARE(positionsIn_(text, u"the"_s, PLAIN_), (QList<int>{ 0, 4, 8 }));
        QCOMPARE(positionsIn_(text, u"the"_s, MATCH_CASE_), (QList<int>{ 4 }));
        QCOMPARE(positionsIn_(text, u"THE"_s, MATCH_CASE_), (QList<int>{ 8 }));
    }

    void wholeWordLeavesLongerWordsAlone()
    {
        auto text = u"Al, Although Al's pal Al"_s;

        QCOMPARE(
            positionsIn_(text, u"Al"_s, PLAIN_),
            (QList<int>{ 0, 4, 13, 19, 22 }));

        // Not "Although" and not "pal". An apostrophe is a boundary, so the
        // one in "Al's" counts
        QCOMPARE(
            positionsIn_(text, u"Al"_s, WHOLE_WORD_),
            (QList<int>{ 0, 13, 22 }));
    }

    void wholeWordAndMatchCaseCombine()
    {
        QCOMPARE(
            positionsIn_(u"al Al all AL"_s, u"Al"_s, Options{ true, true }),
            (QList<int>{ 3 }));
    }

    void matchesDoNotOverlap()
    {
        QCOMPARE(
            positionsIn_(u"aaaaa"_s, u"aa"_s, PLAIN_),
            (QList<int>{ 0, 2 }));
    }

    void aMatchLiesWithinOneLine()
    {
        // "one" then "two" with only a line break between them
        QVERIFY(positionsIn_(u"one\ntwo"_s, u"onetwo"_s, PLAIN_).isEmpty());
        QVERIFY(positionsIn_(u"one\ntwo"_s, u"one two"_s, PLAIN_).isEmpty());
    }

    void aSpaceAndANoBreakSpaceMatchEachOther()
    {
        auto with_no_break = u"one"_s + noBreakSpace_() + u"two"_s;
        auto text = with_no_break + u" one two"_s;

        QList<int> both{ 0, 8 };

        QCOMPARE(positionsIn_(text, u"one two"_s, PLAIN_), both);
        QCOMPARE(positionsIn_(text, with_no_break, PLAIN_), both);
    }

    void replacingLeavesOtherNoBreakSpacesAsTheyAre()
    {
        auto text =
            u"a"_s + noBreakSpace_() + u"b one"_s + noBreakSpace_() + u"two"_s;

        QTextDocument document{};
        setUp_(document, text);

        QCOMPARE(
            TextSearch::replaceAll(&document, u"one two"_s, u"x"_s, PLAIN_),
            1);
        QCOMPARE(textOf_(document), u"a"_s + noBreakSpace_() + u"b x"_s);
    }

    void positionsCountAsACursorDoes()
    {
        // The emoji takes two positions
        QCOMPARE(
            positionsIn_(emoji_() + u"one"_s, u"one"_s, PLAIN_),
            (QList<int>{ 2 }));
    }

    // --- Stepping through matches -------------------------------------------

    void nextIndexIsTheFirstMatchAtOrAfterThePosition()
    {
        QList<Match> matches{
            { 2,  3 },
            { 10, 3 },
            { 20, 3 }
        };

        QCOMPARE(TextSearch::nextIndex(matches, 0), 0);
        QCOMPARE(TextSearch::nextIndex(matches, 2), 0);
        QCOMPARE(TextSearch::nextIndex(matches, 3), 1);
        QCOMPARE(TextSearch::nextIndex(matches, 20), 2);
    }

    void nextIndexWrapsToTheTop()
    {
        QList<Match> matches{
            { 2,  3 },
            { 10, 3 }
        };

        QCOMPARE(TextSearch::nextIndex(matches, 11), 0);
    }

    void previousIndexIsTheLastMatchBeforeThePosition()
    {
        QList<Match> matches{
            { 2,  3 },
            { 10, 3 },
            { 20, 3 }
        };

        QCOMPARE(TextSearch::previousIndex(matches, 21), 2);
        QCOMPARE(TextSearch::previousIndex(matches, 20), 1);
        QCOMPARE(TextSearch::previousIndex(matches, 3), 0);
    }

    void previousIndexWrapsToTheBottom()
    {
        QList<Match> matches{
            { 2,  3 },
            { 10, 3 }
        };

        QCOMPARE(TextSearch::previousIndex(matches, 2), 1);
        QCOMPARE(TextSearch::previousIndex(matches, 0), 1);
    }

    void withNoMatchesThereIsNoIndex()
    {
        QCOMPARE(TextSearch::nextIndex({}, 0), -1);
        QCOMPARE(TextSearch::previousIndex({}, 0), -1);
    }

    // --- Replacing ----------------------------------------------------------

    void replaceOneChangesOnlyThatMatch()
    {
        QTextDocument document{};
        setUp_(document, u"one two one"_s);

        auto matches = TextSearch::findAll(&document, u"one"_s, PLAIN_);
        TextSearch::replaceOne(&document, matches.at(1), u"three"_s);

        QCOMPARE(textOf_(document), u"one two three"_s);
    }

    void replaceAllChangesEveryMatchAndCountsThem()
    {
        QTextDocument document{};
        setUp_(document, u"one two one\nthree one"_s);

        QCOMPARE(
            TextSearch::replaceAll(&document, u"one"_s, u"1"_s, PLAIN_),
            3);
        QCOMPARE(textOf_(document), u"1 two 1\nthree 1"_s);
    }

    void replaceAllWithLongerText()
    {
        QTextDocument document{};
        setUp_(document, u"a b a"_s);

        QCOMPARE(
            TextSearch::replaceAll(&document, u"a"_s, u"alpha"_s, PLAIN_),
            2);
        QCOMPARE(textOf_(document), u"alpha b alpha"_s);
    }

    void replaceAllWithNothingRemovesTheMatches()
    {
        QTextDocument document{};
        setUp_(document, u"one two one"_s);

        QCOMPARE(
            TextSearch::replaceAll(&document, u"one"_s, QString{}, PLAIN_),
            2);
        QCOMPARE(textOf_(document), u" two "_s);
    }

    // "a" becomes "aa" once each: the text a replacement puts in is not
    // searched again
    void replaceAllDoesNotSearchWhatItPutIn()
    {
        QTextDocument document{};
        setUp_(document, u"a a"_s);

        QCOMPARE(TextSearch::replaceAll(&document, u"a"_s, u"aa"_s, PLAIN_), 2);
        QCOMPARE(textOf_(document), u"aa aa"_s);
    }

    void replaceAllHonorsTheOptions()
    {
        QTextDocument document{};
        setUp_(document, u"Al, Although Al's pal"_s);

        QCOMPARE(
            TextSearch::replaceAll(&document, u"Al"_s, u"Bo"_s, WHOLE_WORD_),
            2);
        QCOMPARE(textOf_(document), u"Bo, Although Bo's pal"_s);
    }

    void replaceAllLeavesAloneWhatAlreadyReadsAsTheReplacement()
    {
        QTextDocument document{};
        setUp_(document, u"The the THE"_s);

        // All three match, and the middle one is already "the"
        QCOMPARE(
            TextSearch::replaceAll(&document, u"the"_s, u"the"_s, PLAIN_),
            2);
        QCOMPARE(textOf_(document), u"the the the"_s);
    }

    void replaceAllThatChangesNothingLeavesTheDocumentUnmodified()
    {
        QTextDocument document{};
        setUp_(document, u"one two"_s);

        QCOMPARE(
            TextSearch::replaceAll(&document, u"one"_s, u"one"_s, PLAIN_),
            0);
        QCOMPARE(
            TextSearch::replaceAll(&document, u"three"_s, u"3"_s, PLAIN_),
            0);
        QCOMPARE(
            TextSearch::replaceAll(&document, QString{}, u"3"_s, PLAIN_),
            0);

        QCOMPARE(textOf_(document), u"one two"_s);
        QVERIFY(!document.isModified());
    }

    // --- Through a view, as the app does ------------------------------------

    void replaceOneInAViewReachesTheBufferAndTheOtherView()
    {
        PrimeDocument prime(nullptr);
        prime.setText(u"one two one"_s);

        QTextDocument a{};
        QTextDocument b{};
        a.setDocumentLayout(new QPlainTextDocumentLayout(&a));
        b.setDocumentLayout(new QPlainTextDocumentLayout(&b));
        prime.registerView(&a);
        prime.registerView(&b);

        auto matches = TextSearch::findAll(&a, u"one"_s, PLAIN_);
        TextSearch::replaceOne(&a, matches.at(0), u"three"_s);

        QCOMPARE(prime.text(), u"three two one"_s);
        QCOMPARE(textOf_(b), u"three two one"_s);

        prime.undo();

        QCOMPARE(prime.text(), u"one two one"_s);
        QCOMPARE(textOf_(a), u"one two one"_s);
    }

    void replaceAllInAViewIsOneUndoStep()
    {
        auto before = u"one two one\nthree one\n\nfour"_s;
        auto after = u"eleven two eleven\nthree eleven\n\nfour"_s;

        PrimeDocument prime(nullptr);
        prime.setText(before);

        QTextDocument a{};
        QTextDocument b{};
        a.setDocumentLayout(new QPlainTextDocumentLayout(&a));
        b.setDocumentLayout(new QPlainTextDocumentLayout(&b));
        prime.registerView(&a);
        prime.registerView(&b);

        QCOMPARE(TextSearch::replaceAll(&a, u"one"_s, u"eleven"_s, PLAIN_), 3);

        QCOMPARE(prime.text(), after);
        QCOMPARE(textOf_(a), after);
        QCOMPARE(textOf_(b), after);

        prime.undo();

        QCOMPARE(prime.text(), before);
        QCOMPARE(textOf_(a), before);
        QCOMPARE(textOf_(b), before);
        QVERIFY(!prime.isUndoAvailable());

        prime.redo();

        QCOMPARE(prime.text(), after);
        QCOMPARE(textOf_(b), after);
    }

    void replaceAllInAViewAfterTypingUndoesApartFromTheTyping()
    {
        PrimeDocument prime(nullptr);
        prime.setText(u"one two"_s);

        QTextDocument a{};
        a.setDocumentLayout(new QPlainTextDocumentLayout(&a));
        prime.registerView(&a);

        QTextCursor cursor(&a);
        cursor.movePosition(QTextCursor::End);
        cursor.insertText(u" one"_s);

        QCOMPARE(TextSearch::replaceAll(&a, u"one"_s, u"1"_s, PLAIN_), 2);
        QCOMPARE(prime.text(), u"1 two 1"_s);

        prime.undo();
        QCOMPARE(prime.text(), u"one two one"_s);

        prime.undo();
        QCOMPARE(prime.text(), u"one two"_s);
    }
};

QTEST_MAIN(TextSearchTest)

#include "TextSearchTest.moc"
