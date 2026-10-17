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

#include <optional>

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QTest>
#include <QUrl>

#include "core/UpdateCheck.h"

using namespace Qt::StringLiterals;
using namespace Fernanda;

// Whether a newer release exists (core/UpdateCheck.h): how two versions
// compare, and which release in a list GitHub returned is offered. A list is
// built here in GitHub's shape, holding only the fields the check reads.
//
// Qt Test runs every private slot as a test. A slot named <test>_data fills a
// table, and <test> then runs once per row, reported under the row's name
class UpdateCheckTest : public QObject
{
    Q_OBJECT

private:
    [[nodiscard]] static QJsonObject
    release_(const QString& tag, bool draft = false)
    {
        return QJsonObject{
            { u"tag_name"_s, tag                                         },
            { u"html_url"_s,
             u"https://github.com/fairybow/Suzuri/releases/tag/"_s + tag },
            { u"draft"_s,    draft                                       },
        };
    }

private slots:
    // expected is the sign of compareVersions(a, b): -1 when a is older, 0
    // when the same, 1 when newer
    void compareVersionsFollowsPrecedence_data()
    {
        QTest::addColumn<QString>("a");
        QTest::addColumn<QString>("b");
        QTest::addColumn<int>("expected");

        QTest::newRow("same") << u"1.2.3"_s << u"1.2.3"_s << 0;
        QTest::newRow("major") << u"2.0.0"_s << u"1.9.9"_s << 1;
        QTest::newRow("minor") << u"1.2.0"_s << u"1.10.0"_s << -1;
        QTest::newRow("patch") << u"1.2.4"_s << u"1.2.3"_s << 1;

        // A prerelease leads up to its release
        QTest::newRow("prerelease below release")
            << u"1.0.0-rc.1"_s << u"1.0.0"_s << -1;
        QTest::newRow("prerelease above older release")
            << u"1.1.0-beta.0"_s << u"1.0.0"_s << 1;

        // Identifiers: numbers as numbers, words in ASCII order, numbers
        // below words, and a shorter list below a longer one it begins
        QTest::newRow("numeric identifiers")
            << u"0.6.0-beta.10"_s << u"0.6.0-beta.2"_s << 1;
        QTest::newRow("word identifiers")
            << u"1.0.0-alpha"_s << u"1.0.0-beta"_s << -1;
        QTest::newRow("beta below rc")
            << u"1.0.0-rc.0"_s << u"1.0.0-beta.9"_s << 1;
        QTest::newRow("number below word")
            << u"1.0.0-1"_s << u"1.0.0-alpha"_s << -1;
        QTest::newRow("fewer identifiers below more")
            << u"1.0.0-beta"_s << u"1.0.0-beta.1"_s << -1;
        QTest::newRow("same prerelease")
            << u"0.6.0-beta.0"_s << u"0.6.0-beta.0"_s << 0;

        // A tag's "v" and build metadata take no part
        QTest::newRow("tag against version")
            << u"v0.6.0-beta.0"_s << u"0.6.0-beta.0"_s << 0;
        QTest::newRow("build metadata ignored")
            << u"1.0.0+build.5"_s << u"1.0.0"_s << 0;
    }

    void compareVersionsFollowsPrecedence()
    {
        QFETCH(QString, a);
        QFETCH(QString, b);
        QFETCH(int, expected);

        auto result = UpdateCheck::compareVersions(a, b);
        QVERIFY(result.has_value());

        auto sign = *result < 0 ? -1 : *result > 0 ? 1 : 0;
        QCOMPARE(sign, expected);
    }

    void compareVersionsRejectsWhatIsNotAVersion_data()
    {
        QTest::addColumn<QString>("text");

        QTest::newRow("empty") << u""_s;
        QTest::newRow("two parts") << u"1.2"_s;
        QTest::newRow("four parts") << u"1.2.3.4"_s;
        QTest::newRow("word") << u"latest"_s;
        QTest::newRow("letter in a number") << u"1.2a.3"_s;
        QTest::newRow("empty prerelease") << u"1.2.3-"_s;
        QTest::newRow("empty identifier") << u"1.2.3-beta..1"_s;
        QTest::newRow("space in identifier") << u"1.2.3-be ta"_s;
    }

    void compareVersionsRejectsWhatIsNotAVersion()
    {
        QFETCH(QString, text);

        QVERIFY(!UpdateCheck::compareVersions(text, u"1.0.0"_s).has_value());
        QVERIFY(!UpdateCheck::compareVersions(u"1.0.0"_s, text).has_value());
    }

    void isPrereleaseReadsThePrereleasePart()
    {
        QVERIFY(UpdateCheck::isPrerelease(u"0.6.0-beta.0"_s));
        QVERIFY(UpdateCheck::isPrerelease(u"v1.0.0-rc.1"_s));
        QVERIFY(!UpdateCheck::isPrerelease(u"1.0.0"_s));
        QVERIFY(!UpdateCheck::isPrerelease(u"1.0.0+build.5"_s));
        QVERIFY(!UpdateCheck::isPrerelease(u"not a version"_s));
    }

    // GitHub lists the newest created first, which isn't always the highest
    void availableUpdateOffersTheHighestNotTheFirst()
    {
        QJsonArray releases{
            release_(u"v0.6.1-beta.0"_s),
            release_(u"v0.7.0-beta.0"_s),
            release_(u"v0.6.0-beta.0"_s),
        };

        auto update =
            UpdateCheck::availableUpdate(releases, u"0.6.0-beta.0"_s, true);

        QVERIFY(update.has_value());
        QCOMPARE(update->version, u"0.7.0-beta.0"_s);
        QCOMPARE(
            update->pageUrl,
            QUrl(
                u"https://github.com/fairybow/Suzuri/releases/tag/"
                u"v0.7.0-beta.0"_s));
    }

    void availableUpdateOffersNothingWhenCurrent()
    {
        QJsonArray releases{
            release_(u"v0.6.0-beta.0"_s),
            release_(u"v0.5.0-beta.0"_s),
        };

        QVERIFY(!UpdateCheck::availableUpdate(releases, u"0.6.0-beta.0"_s, true)
                     .has_value());
    }

    // A build newer than anything released (a development build) is offered
    // nothing older
    void availableUpdateOffersNothingOlder()
    {
        QJsonArray releases{ release_(u"v0.6.0-beta.0"_s) };

        QVERIFY(!UpdateCheck::availableUpdate(releases, u"0.7.0-beta.0"_s, true)
                     .has_value());
    }

    void availableUpdatePassesOverDrafts()
    {
        QJsonArray releases{
            release_(u"v0.8.0-beta.0"_s, true),
            release_(u"v0.7.0-beta.0"_s),
        };

        auto update =
            UpdateCheck::availableUpdate(releases, u"0.6.0-beta.0"_s, true);

        QVERIFY(update.has_value());
        QCOMPARE(update->version, u"0.7.0-beta.0"_s);
    }

    void availableUpdatePassesOverTagsThatAreNotVersions()
    {
        QJsonArray releases{
            release_(u"nightly"_s),
            release_(u"v0.7.0-beta.0"_s),
        };

        auto update =
            UpdateCheck::availableUpdate(releases, u"0.6.0-beta.0"_s, true);

        QVERIFY(update.has_value());
        QCOMPARE(update->version, u"0.7.0-beta.0"_s);
    }

    void availableUpdateLeavesOutPrereleasesWhenAsked()
    {
        QJsonArray releases{
            release_(u"v1.1.0-beta.0"_s),
            release_(u"v1.0.1"_s),
        };

        auto without =
            UpdateCheck::availableUpdate(releases, u"1.0.0"_s, false);
        QVERIFY(without.has_value());
        QCOMPARE(without->version, u"1.0.1"_s);

        auto with = UpdateCheck::availableUpdate(releases, u"1.0.0"_s, true);
        QVERIFY(with.has_value());
        QCOMPARE(with->version, u"1.1.0-beta.0"_s);
    }

    void availableUpdateOffersAReleaseToAPrerelease()
    {
        QJsonArray releases{ release_(u"v1.0.0"_s) };

        auto update =
            UpdateCheck::availableUpdate(releases, u"1.0.0-rc.2"_s, false);

        QVERIFY(update.has_value());
        QCOMPARE(update->version, u"1.0.0"_s);
    }

    void availableUpdateOffersNothingFromAnEmptyList()
    {
        QVERIFY(!UpdateCheck::availableUpdate({}, u"0.6.0-beta.0"_s, true)
                     .has_value());
    }
};

QTEST_GUILESS_MAIN(UpdateCheckTest)

#include "UpdateCheckTest.moc"
