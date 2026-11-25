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
#include <QObject>
#include <QTest>

#include "ui/tabs/BreadcrumbFit.h"

using Fernanda::Ui::Breadcrumbs::fit;

// How a path's parts share a header's width (ui/tabs/BreadcrumbFit.h): the
// name first, the folders sharing what's left, and folders dropped from the
// inside out when even their minimums don't fit. Widths are plain numbers.
// In each test the separator is 10 wide and the ellipsis 5.
//
// Qt Test runs every private slot as a test
class BreadcrumbFitTest : public QObject
{
    Q_OBJECT

private:
    static constexpr int SEPARATOR_ = 10;
    static constexpr int ELLIPSIS_ = 5;

private slots:
    void everythingFitsWhole()
    {
        auto out = fit(200, 50, { 30, 40 }, { 8, 8 }, SEPARATOR_, ELLIPSIS_);

        QCOMPARE(out.name, 50);
        QCOMPARE(out.folders, (QList<int>{ 30, 40 }));
        QVERIFY(!out.dropped);
    }

    // The exact width is enough
    void everythingFitsExactly()
    {
        auto out = fit(140, 50, { 30, 40 }, { 8, 8 }, SEPARATOR_, ELLIPSIS_);

        QCOMPARE(out.folders, (QList<int>{ 30, 40 }));
    }

    // A name as wide as the header or wider is shortened, and no folder shows
    void aNameThatDoesntFitIsShortenedAlone()
    {
        auto out = fit(40, 50, { 30 }, { 8 }, SEPARATOR_, ELLIPSIS_);

        QCOMPARE(out.name, 40);
        QVERIFY(out.folders.isEmpty());
        QVERIFY(!out.dropped);
    }

    void aPathWithNoFoldersIsItsName()
    {
        auto out = fit(200, 50, {}, {}, SEPARATOR_, ELLIPSIS_);

        QCOMPARE(out.name, 50);
        QVERIFY(out.folders.isEmpty());
    }

    // 100 left after the name, 20 of it separators: two equal shares of 40
    void foldersShortenToEqualShares()
    {
        auto out = fit(150, 50, { 60, 70 }, { 8, 8 }, SEPARATOR_, ELLIPSIS_);

        QCOMPARE(out.name, 50);
        QCOMPARE(out.folders, (QList<int>{ 40, 40 }));
    }

    // A folder narrower than its share keeps its width, and the others share
    // the rest: 80 for folders, the short one 20, the others 30 each
    void aShortFolderKeepsItsWidth()
    {
        auto out =
            fit(160, 50, { 20, 60, 70 }, { 8, 8, 8 }, SEPARATOR_, ELLIPSIS_);

        QCOMPARE(out.folders, (QList<int>{ 20, 30, 30 }));
    }

    // A remainder that doesn't divide evenly goes a pixel at a time to the
    // first folders
    void aRemainderGoesToTheFirstFolders()
    {
        auto out = fit(141, 50, { 60, 70 }, { 8, 8 }, SEPARATOR_, ELLIPSIS_);

        QCOMPARE(out.folders, (QList<int>{ 36, 35 }));
    }

    // A folder whose minimum is more than its share keeps its minimum, and
    // the other takes what's left
    void noFolderIsShorterThanItsMinimum()
    {
        auto out = fit(130, 50, { 60, 70 }, { 40, 8 }, SEPARATOR_, ELLIPSIS_);

        QCOMPARE(out.folders, (QList<int>{ 40, 20 }));
    }

    // A folder held at its minimum lowers the share for the rest, so a folder
    // that fit the first share whole may not fit what's left. 72 is left
    // after the name, 42 of it for folders: the first keeps its minimum of
    // 18, and the other two share the 24 left, though each fit the first
    // share of 14 whole
    void aMinimumHeldLowersTheShareForTheRest()
    {
        auto out =
            fit(122,
                50,
                { 100, 14, 14 },
                { 18, 10, 10 },
                SEPARATOR_,
                ELLIPSIS_);

        QCOMPARE(out.name, 50);
        QCOMPARE(out.folders, (QList<int>{ 18, 12, 12 }));
        QVERIFY(!out.dropped);
    }

    // Not every minimum fits, so the innermost folders go and an ellipsis
    // stands for them. 55 is left after the name: three folders at their
    // minimums need 60, and two with the ellipsis need 55
    void innerFoldersAreDroppedFirst()
    {
        auto out =
            fit(105, 50, { 60, 60, 60 }, { 10, 10, 10 }, SEPARATOR_, ELLIPSIS_);

        QCOMPARE(out.name, 50);
        QCOMPARE(out.folders, (QList<int>{ 10, 10 }));
        QVERIFY(out.dropped);
    }

    // The outermost folders are the ones kept, told apart here by their
    // minimums. 85 is left after the name: all three need 90, the first two
    // with the ellipsis 85. The first keeps its minimum of 40, more than an
    // even share, and the second takes the 10 left
    void theOutermostFoldersAreKept()
    {
        auto out =
            fit(135, 50, { 60, 60, 60 }, { 40, 10, 10 }, SEPARATOR_, ELLIPSIS_);

        QCOMPARE(out.folders, (QList<int>{ 40, 10 }));
        QVERIFY(out.dropped);
    }

    // Room for the ellipsis and nothing else: every folder goes
    void onlyTheEllipsisFits()
    {
        auto out = fit(70, 50, { 60, 60 }, { 10, 10 }, SEPARATOR_, ELLIPSIS_);

        QCOMPARE(out.name, 50);
        QVERIFY(out.folders.isEmpty());
        QVERIFY(out.dropped);
    }

    // Not even the ellipsis fits: just the name
    void withNoRoomForFoldersOnlyTheNameShows()
    {
        auto out = fit(60, 50, { 60, 60 }, { 10, 10 }, SEPARATOR_, ELLIPSIS_);

        QCOMPARE(out.name, 50);
        QVERIFY(out.folders.isEmpty());
        QVERIFY(!out.dropped);
    }

    // A folder shorter than its minimum (one letter) is drawn whole
    void aMinimumNeverWidensAFolder()
    {
        auto out = fit(100, 50, { 6, 70 }, { 10, 10 }, SEPARATOR_, ELLIPSIS_);

        QCOMPARE(out.folders, (QList<int>{ 6, 24 }));
    }

    void noWidthDrawsNothing()
    {
        auto out = fit(0, 50, { 30 }, { 8 }, SEPARATOR_, ELLIPSIS_);

        QCOMPARE(out.name, 0);
        QVERIFY(out.folders.isEmpty());
    }
};

QTEST_MAIN(BreadcrumbFitTest)

#include "BreadcrumbFitTest.moc"
