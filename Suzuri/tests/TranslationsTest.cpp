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
#include <QString>
#include <QStringList>
#include <QTest>

#include "core/Translations.h"

using namespace Qt::StringLiterals;
using Suzuri::Translations;

// Which language the interface uses (core/Translations.h): the setting when
// there's a translation for it, else the system's preferred languages in
// order, else English. The available languages are given here, not read from
// the embedded catalogs, which the tests don't have.
//
// Qt Test runs every private slot as a test. A slot named <test>_data fills a
// table, and <test> then runs once per row, reported under the row's name
class TranslationsTest : public QObject
{
    Q_OBJECT

private slots:
    void pickChoosesTheLanguage_data()
    {
        QTest::addColumn<QString>("setting");
        QTest::addColumn<QStringList>("uiLanguages");
        QTest::addColumn<QString>("expected");

        auto english = QStringList{ u"en-US"_s, u"en"_s };
        auto spanish = QStringList{ u"es-MX"_s, u"es"_s };

        QTest::newRow("system English") << u""_s << english << u"en"_s;
        QTest::newRow("system Spanish") << u""_s << spanish << u"es"_s;
        QTest::newRow("setting over system") << u"es"_s << english << u"es"_s;
        QTest::newRow("English setting over Spanish system")
            << u"en"_s << spanish << u"en"_s;

        // A setting with no translation falls to the system's
        QTest::newRow("unavailable setting") << u"fr"_s << spanish << u"es"_s;

        // The first preferred language with a translation wins
        QTest::newRow("later preference")
            << u""_s << QStringList{ u"fr-FR"_s, u"es-ES"_s, u"en-US"_s }
            << u"es"_s;

        // Nothing available falls to English
        QTest::newRow("no preference available")
            << u""_s << QStringList{ u"de-DE"_s, u"fr"_s } << u"en"_s;
        QTest::newRow("no preferences") << u""_s << QStringList{} << u"en"_s;

        // Qt reports "C" when the system names no language
        QTest::newRow("C locale") << u""_s << QStringList{ u"C"_s } << u"en"_s;

        // Region separators and case don't matter
        QTest::newRow("underscore")
            << u""_s << QStringList{ u"es_AR"_s } << u"es"_s;
        QTest::newRow("upper case")
            << u""_s << QStringList{ u"ES"_s } << u"es"_s;
    }

    void pickChoosesTheLanguage()
    {
        QFETCH(QString, setting);
        QFETCH(QStringList, uiLanguages);
        QFETCH(QString, expected);

        auto available = QStringList{ u"en"_s, u"es"_s };

        QCOMPARE(Translations::pick(available, setting, uiLanguages), expected);
    }

    void displayNameUsesTheLanguagesOwnName()
    {
        QCOMPARE(Translations::displayName(u"en"_s), u"English"_s);
        QCOMPARE(Translations::displayName(u"es"_s), u"Español"_s);
    }
};

QTEST_GUILESS_MAIN(TranslationsTest)

#include "TranslationsTest.moc"
