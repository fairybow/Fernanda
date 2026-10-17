/*
 * Suzuri — A plain-text writing app for fiction, poetry, and long projects
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
#include <QPointer>
#include <QSignalSpy>
#include <QTest>
#include <QWidget>

#include "ui/tabs/TabPaneLeaf.h"

using namespace Qt::StringLiterals;
using Suzuri::Ui::TabPaneLeaf;

// A leaf and the lifetimes of its pages (ui/tabs/TabPaneLeaf.h): a page that
// dies on its own takes its tab with it, and a leaf that dies takes its pages
// with it, quietly.
//
// The second is also a check for a sanitizer build (-fsanitize=undefined): a
// page's handler running on a leaf already partly destroyed is undefined
// behavior, reported there, though it may well pass here.
//
// Qt Test runs every private slot as a test
class TabPaneLeafTest : public QObject
{
    Q_OBJECT

private slots:
    void aPageDeletedOnItsOwnTakesItsTab()
    {
        TabPaneLeaf leaf{};
        auto* first = new QWidget;
        auto* second = new QWidget;
        leaf.addPage(first, u"First"_s);
        leaf.addPage(second, u"Second"_s);

        QSignalSpy count_changed(&leaf, &TabPaneLeaf::pageCountChanged);
        delete first;

        QCOMPARE(leaf.count(), 1);
        QCOMPARE(leaf.pageAt(0), second);
        QCOMPARE(count_changed.count(), 1);
    }

    void aLeafDestroyedWithPagesTakesThemQuietly()
    {
        auto* leaf = new TabPaneLeaf;
        QPointer<QWidget> first = new QWidget;
        QPointer<QWidget> second = new QWidget;
        leaf->addPage(first, u"First"_s);
        leaf->addPage(second, u"Second"_s);

        auto count_changes = 0;
        connect(leaf, &TabPaneLeaf::pageCountChanged, this, [&count_changes] {
            ++count_changes;
        });

        delete leaf;

        QVERIFY(first.isNull());
        QVERIFY(second.isNull());
        QCOMPARE(count_changes, 0);
    }
};

QTEST_MAIN(TabPaneLeafTest)

#include "TabPaneLeafTest.moc"
