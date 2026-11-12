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

#include <QLocale>
#include <QObject>
#include <QString>
#include <QTest>

#include "core/VaultConfig.h"

using namespace Qt::StringLiterals;
using Fernanda::VaultConfig;

// The spelling language a vault checks until one is chosen
// (core/VaultConfig.h): British English for an English locale outside the
// United States, US English for any other. The locale is given here, not read
// from the system, which differs from machine to machine.
//
// Qt Test runs every private slot as a test. A slot named <test>_data fills a
// table, and <test> then runs once per row, reported under the row's name
class VaultConfigTest : public QObject
{
    Q_OBJECT

private slots:
    void defaultSpellcheckLanguageFollowsTheLocale_data()
    {
        QTest::addColumn<QString>("locale");
        QTest::addColumn<QString>("expected");

        QTest::newRow("United States") << u"en_US"_s << u"en_US"_s;
        QTest::newRow("United Kingdom") << u"en_GB"_s << u"en_GB"_s;
        QTest::newRow("Singapore") << u"en_SG"_s << u"en_GB"_s;
        QTest::newRow("Australia") << u"en_AU"_s << u"en_GB"_s;
        QTest::newRow("India") << u"en_IN"_s << u"en_GB"_s;

        // English with no territory is taken as the United States, as QLocale
        // takes it
        QTest::newRow("English alone") << u"en"_s << u"en_US"_s;

        // Another language has no bundled dictionary of its own
        QTest::newRow("Spanish") << u"es_MX"_s << u"en_US"_s;
        QTest::newRow("C locale") << u"C"_s << u"en_US"_s;
    }

    void defaultSpellcheckLanguageFollowsTheLocale()
    {
        QFETCH(QString, locale);
        QFETCH(QString, expected);

        QCOMPARE(
            VaultConfig::defaultSpellcheckLanguage(QLocale(locale)),
            expected);
    }
};

QTEST_GUILESS_MAIN(VaultConfigTest)

#include "VaultConfigTest.moc"
