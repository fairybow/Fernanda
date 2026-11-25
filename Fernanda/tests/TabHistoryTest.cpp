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

#include <optional>

#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QTemporaryDir>
#include <QTest>
#include <QWidget>
#include <QtTypes>

#include <Coco/Path.h>

#include "core/files/PathKey.h"
#include "core/git/GitStatus.h"
#include "ui/tabs/TabHistory.h"

using namespace Qt::StringLiterals;

using Fernanda::PathKey;
using Fernanda::Ui::TabHistory;

// A tab's back and forward history (ui/tabs/TabHistory.h): the order entries
// come back in, what a new page does to the forward list, the limit, how file
// entries follow a rename, and the history riding on a page widget.
//
// Qt Test runs every private slot as a test
class TabHistoryTest : public QObject
{
    Q_OBJECT

private:
    using Entry_ = TabHistory::Entry;

    // Keys under a temporary folder, so they're absolute on every platform
    struct Fixture_
    {
        QTemporaryDir folder{};

        [[nodiscard]] PathKey key(const QString& relative) const
        {
            return PathKey::of(Coco::Path(folder.filePath(relative)));
        }

        [[nodiscard]] Entry_ file(const QString& relative) const
        {
            Entry_ entry{};
            entry.kind = Entry_::Kind::File;
            entry.file = key(relative);
            return entry;
        }
    };

    [[nodiscard]] static Entry_ diff_(const QString& relative)
    {
        Entry_ entry{};
        entry.kind = Entry_::Kind::Diff;
        entry.change.path = Coco::Path(relative);
        return entry;
    }

    // The file an entry names, or an empty key when there's no entry
    [[nodiscard]] static PathKey fileOf_(const std::optional<Entry_>& entry)
    {
        return entry ? entry->file : PathKey{};
    }

private slots:
    void recordPutsThePageOnTheBackList()
    {
        Fixture_ f{};
        TabHistory history{};

        history.record(f.file(u"a.txt"_s));

        QVERIFY(fileOf_(history.takeBack()) == f.key(u"a.txt"_s));
        QVERIFY(!history.takeBack());
        QVERIFY(!history.takeForward());
    }

    void theBackListGivesTheNewestFirst()
    {
        Fixture_ f{};
        TabHistory history{};

        history.record(f.file(u"a.txt"_s));
        history.record(f.file(u"b.txt"_s));

        QVERIFY(fileOf_(history.takeBack()) == f.key(u"b.txt"_s));
        QVERIFY(fileOf_(history.takeBack()) == f.key(u"a.txt"_s));
    }

    void theForwardListGivesTheNewestFirst()
    {
        Fixture_ f{};
        TabHistory history{};

        history.pushForward(f.file(u"a.txt"_s));
        history.pushForward(f.file(u"b.txt"_s));

        QVERIFY(fileOf_(history.takeForward()) == f.key(u"b.txt"_s));
        QVERIFY(fileOf_(history.takeForward()) == f.key(u"a.txt"_s));
        QVERIFY(!history.takeForward());
    }

    // A new page after going back starts a new line, as in a browser
    void recordClearsTheForwardList()
    {
        Fixture_ f{};
        TabHistory history{};

        history.pushForward(f.file(u"a.txt"_s));
        history.record(f.file(u"b.txt"_s));

        QVERIFY(!history.takeForward());
    }

    // Going forward puts the page it leaves on the back list, and the rest of
    // the forward list stays
    void pushBackKeepsTheForwardList()
    {
        Fixture_ f{};
        TabHistory history{};

        history.pushForward(f.file(u"a.txt"_s));
        history.pushBack(f.file(u"b.txt"_s));

        QVERIFY(fileOf_(history.takeForward()) == f.key(u"a.txt"_s));
        QVERIFY(fileOf_(history.takeBack()) == f.key(u"b.txt"_s));
    }

    void eachListKeepsOnlyTheNewestEntries()
    {
        Fixture_ f{};
        TabHistory history{};

        const auto total = TabHistory::MAX_ENTRIES + 5;

        for (qsizetype i = 0; i < total; ++i) {
            history.record(f.file(u"%1.txt"_s.arg(i)));
        }

        for (qsizetype i = 0; i < total; ++i) {
            history.pushForward(f.file(u"f%1.txt"_s.arg(i)));
        }

        for (auto i = total - 1; i >= total - TabHistory::MAX_ENTRIES; --i) {
            QVERIFY(fileOf_(history.takeBack()) == f.key(u"%1.txt"_s.arg(i)));
            QVERIFY(
                fileOf_(history.takeForward()) == f.key(u"f%1.txt"_s.arg(i)));
        }

        QVERIFY(!history.takeBack());
        QVERIFY(!history.takeForward());
    }

    // A file's own rename, and a folder's, on both lists
    void rebaseFollowsAFileOrItsFolder()
    {
        Fixture_ f{};
        TabHistory history{};

        history.record(f.file(u"a.txt"_s));
        history.record(f.file(u"one/b.txt"_s));
        history.record(f.file(u"other.txt"_s));
        history.pushForward(f.file(u"one/two/c.txt"_s));

        history.rebase(f.key(u"a.txt"_s), f.key(u"z.txt"_s));
        history.rebase(f.key(u"one"_s), f.key(u"uno"_s));

        QVERIFY(fileOf_(history.takeForward()) == f.key(u"uno/two/c.txt"_s));
        QVERIFY(fileOf_(history.takeBack()) == f.key(u"other.txt"_s));
        QVERIFY(fileOf_(history.takeBack()) == f.key(u"uno/b.txt"_s));
        QVERIFY(fileOf_(history.takeBack()) == f.key(u"z.txt"_s));
    }

    // "one/a.txt" is not under "one/a"
    void rebaseComparesWholeNames()
    {
        Fixture_ f{};
        TabHistory history{};

        history.record(f.file(u"one/a.txt"_s));

        history.rebase(f.key(u"one/a"_s), f.key(u"one/b"_s));

        QVERIFY(fileOf_(history.takeBack()) == f.key(u"one/a.txt"_s));
    }

    void rebaseLeavesADiffAlone()
    {
        Fixture_ f{};
        TabHistory history{};

        history.record(diff_(u"one/a.txt"_s));

        history.rebase(f.key(u"one"_s), f.key(u"uno"_s));

        auto entry = history.takeBack();
        QVERIFY(entry);
        QVERIFY(entry->kind == Entry_::Kind::Diff);
        QCOMPARE(entry->change.path, Coco::Path(u"one/a.txt"_s));
        QVERIFY(entry->file.isEmpty());
    }

    // An entry keeps the view's state as it was given
    void anEntryKeepsItsViewState()
    {
        Fixture_ f{};
        TabHistory history{};

        auto entry = f.file(u"a.txt"_s);
        entry.viewState[u"cursor"_s] = 42;
        history.record(entry);

        auto back = history.takeBack();
        QVERIFY(back);
        QCOMPARE(back->viewState.value(u"cursor"_s).toInt(), 42);
    }

    // The history rides on the page widget, as a copy
    void aPageCarriesItsHistory()
    {
        Fixture_ f{};
        QWidget page{};

        QVERIFY(!Fernanda::Ui::pageHistory(&page).takeBack());
        QVERIFY(!Fernanda::Ui::pageHistory(nullptr).takeBack());

        TabHistory history{};
        history.record(f.file(u"a.txt"_s));
        Fernanda::Ui::setPageHistory(&page, history);

        auto read = Fernanda::Ui::pageHistory(&page);
        QVERIFY(fileOf_(read.takeBack()) == f.key(u"a.txt"_s));

        // Taking from the copy left the page's own history as it was
        QVERIFY(
            fileOf_(Fernanda::Ui::pageHistory(&page).takeBack()) ==
            f.key(u"a.txt"_s));
    }
};

QTEST_MAIN(TabHistoryTest)

#include "TabHistoryTest.moc"
