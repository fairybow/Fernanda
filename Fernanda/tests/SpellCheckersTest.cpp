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

#include <QByteArray>
#include <QChar>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QTest>

#include <Coco/Path.h>

#include "core/spell/SpellChecker.h"
#include "core/spell/SpellCheckers.h"

using namespace Qt::StringLiterals;
using Fernanda::SpellCheckers;

// Which dictionaries a folder offers, and the checkers made from them
// (core/spell/SpellCheckers.h). The first tests write small dictionaries of
// their own. The last ones install the dictionary that ships with Fernanda,
// which is compiled into this test as it is into the app, and check it against
// a few English words.
//
// Qt Test runs every private slot as a test
class SpellCheckersTest : public QObject
{
    Q_OBJECT

private:
    static void write_(
        const QTemporaryDir& folder,
        const QString& name,
        const QByteArray& bytes)
    {
        QFile file(folder.filePath(name));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(bytes);
    }

    // A dictionary of one word, under the given language name
    static void writeDictionary_(
        const QTemporaryDir& folder,
        const QString& language,
        const QByteArray& word)
    {
        write_(folder, language + u".aff"_s, "SET UTF-8\n");
        write_(folder, language + u".dic"_s, "1\n" + word + "\n");
    }

private slots:
    // --- Languages ----------------------------------------------------------

    void languagesAreTheNamesWithBothFiles()
    {
        QTemporaryDir folder{};
        writeDictionary_(folder, u"xx_YY"_s, "hello");
        writeDictionary_(folder, u"aa_BB"_s, "hallo");
        write_(folder, u"only_affix.aff"_s, "SET UTF-8\n");
        write_(folder, u"only_words.dic"_s, "1\nhello\n");
        write_(folder, u"notes.txt"_s, "not a dictionary");

        SpellCheckers checkers{ Coco::Path(folder.path()) };

        QCOMPARE(checkers.languages(), (QStringList{ u"aa_BB"_s, u"xx_YY"_s }));
    }

    void anEmptyOrMissingFolderHasNoLanguages()
    {
        QTemporaryDir folder{};

        SpellCheckers empty{ Coco::Path(folder.path()) };
        QVERIFY(empty.languages().isEmpty());

        SpellCheckers missing{ Coco::Path(folder.filePath(u"none"_s)) };
        QVERIFY(missing.languages().isEmpty());
        QCOMPARE(missing.checker(u"xx_YY"_s), nullptr);
    }

    void aDictionaryAddedLaterIsFound()
    {
        QTemporaryDir folder{};
        SpellCheckers checkers{ Coco::Path(folder.path()) };
        QVERIFY(checkers.languages().isEmpty());

        writeDictionary_(folder, u"xx_YY"_s, "hello");

        QCOMPARE(checkers.languages(), QStringList{ u"xx_YY"_s });
        QVERIFY(checkers.checker(u"xx_YY"_s));
    }

    // --- Checkers -----------------------------------------------------------

    void aCheckerUsesItsOwnLanguagesWords()
    {
        QTemporaryDir folder{};
        writeDictionary_(folder, u"xx_YY"_s, "hello");
        writeDictionary_(folder, u"aa_BB"_s, "hallo");

        SpellCheckers checkers{ Coco::Path(folder.path()) };

        auto* first = checkers.checker(u"xx_YY"_s);
        auto* second = checkers.checker(u"aa_BB"_s);

        QVERIFY(first);
        QVERIFY(second);
        QVERIFY(first->isValid());
        QVERIFY(first->isCorrect(u"hello"_s));
        QVERIFY(!first->isCorrect(u"hallo"_s));
        QVERIFY(second->isCorrect(u"hallo"_s));
        QVERIFY(!second->isCorrect(u"hello"_s));
    }

    void aLanguageHasOneCheckerHoweverOftenItIsAskedFor()
    {
        QTemporaryDir folder{};
        writeDictionary_(folder, u"xx_YY"_s, "hello");

        SpellCheckers checkers{ Coco::Path(folder.path()) };

        auto* first = checkers.checker(u"xx_YY"_s);
        QVERIFY(first);
        QCOMPARE(checkers.checker(u"xx_YY"_s), first);
    }

    void aLanguageWithNoDictionaryHasNoChecker()
    {
        QTemporaryDir folder{};
        writeDictionary_(folder, u"xx_YY"_s, "hello");
        write_(folder, u"only_affix.aff"_s, "SET UTF-8\n");

        SpellCheckers checkers{ Coco::Path(folder.path()) };

        QCOMPARE(checkers.checker(u"zz_ZZ"_s), nullptr);
        QCOMPARE(checkers.checker(u"only_affix"_s), nullptr);
        QCOMPARE(checkers.checker(QString{}), nullptr);
    }

    void aLanguageIsUsableWhenItsEncodingConverts()
    {
        QTemporaryDir folder{};
        writeDictionary_(folder, u"xx_YY"_s, "hello");
        write_(folder, u"zz_ZZ.aff"_s, "SET X-NO-SUCH-ENCODING\n");
        write_(folder, u"zz_ZZ.dic"_s, "1\nhello\n");
        write_(folder, u"only_affix.aff"_s, "SET UTF-8\n");

        SpellCheckers checkers{ Coco::Path(folder.path()) };

        QVERIFY(checkers.isUsable(u"xx_YY"_s));
        QVERIFY(!checkers.isUsable(u"zz_ZZ"_s));
        QVERIFY(!checkers.isUsable(u"only_affix"_s));
        QVERIFY(!checkers.isUsable(u"none"_s));

        // Still a language, and its checker is the one that isn't valid
        QVERIFY(checkers.languages().contains(u"zz_ZZ"_s));
        QVERIFY(!checkers.checker(u"zz_ZZ"_s)->isValid());
    }

    // A language is a file name in the folder, so one that names a file
    // anywhere else is refused, even when there is a dictionary there
    void aLanguageMustBeASingleName()
    {
        QTemporaryDir outer{};
        writeDictionary_(outer, u"outside"_s, "hello");

        QDir(outer.path()).mkpath(u"dictionaries/sub"_s);
        auto folder = Coco::Path(outer.filePath(u"dictionaries"_s));

        QFile::copy(
            outer.filePath(u"outside.aff"_s),
            outer.filePath(u"dictionaries/sub/inside.aff"_s));
        QFile::copy(
            outer.filePath(u"outside.dic"_s),
            outer.filePath(u"dictionaries/sub/inside.dic"_s));

        SpellCheckers checkers{ folder };

        for (const auto& language :
             { u"../outside"_s, u"sub/inside"_s, u"."_s, u".."_s }) {
            QCOMPARE(checkers.checker(language), nullptr);
            QVERIFY(!checkers.isUsable(language));
        }
    }

    // --- The dictionary that ships with Fernanda ----------------------------

    void installBundledPutsEnglishInTheFolder()
    {
        QTemporaryDir folder{};
        auto path = Coco::Path(folder.path());

        SpellCheckers::installBundled(path);

        SpellCheckers checkers{ path };
        QVERIFY(checkers.languages().contains(u"en_US"_s));

        // The folder is the user's to manage, so the copies can be written
        QVERIFY(QFileInfo(folder.filePath(u"en_US.aff"_s)).isWritable());
        QVERIFY(QFileInfo(folder.filePath(u"en_US.dic"_s)).isWritable());
    }

    void installBundledMakesTheFolderIfItIsMissing()
    {
        QTemporaryDir folder{};
        auto path = Coco::Path(folder.filePath(u"made/later"_s));

        SpellCheckers::installBundled(path);

        QVERIFY(path.isDir());
        QVERIFY(SpellCheckers{ path }.languages().contains(u"en_US"_s));
    }

    void installBundledLeavesAFileThatIsAlreadyThere()
    {
        QTemporaryDir folder{};
        write_(folder, u"en_US.aff"_s, "SET UTF-8\n");
        write_(folder, u"en_US.dic"_s, "1\nxyzzy\n");

        auto path = Coco::Path(folder.path());
        SpellCheckers::installBundled(path);

        SpellCheckers checkers{ path };
        auto* english = checkers.checker(u"en_US"_s);

        QVERIFY(english);
        QVERIFY(english->isCorrect(u"xyzzy"_s));
        QVERIFY(!english->isCorrect(u"hello"_s));
    }

    void theBundledDictionaryKnowsEnglish()
    {
        QTemporaryDir folder{};
        auto path = Coco::Path(folder.path());
        SpellCheckers::installBundled(path);

        SpellCheckers checkers{ path };
        auto* english = checkers.checker(u"en_US"_s);

        QVERIFY(english);
        QVERIFY(english->isValid());

        QVERIFY(english->isCorrect(u"hello"_s));
        QVERIFY(english->isCorrect(u"Westphalia"_s));
        QVERIFY(english->isCorrect(u"magnificent"_s));
        QVERIFY(english->isCorrect(u"don't"_s));
        QVERIFY(english->isCorrect(u"don"_s + QChar(0x2019) + u"t"_s));
        QVERIFY(english->isCorrect(u"well-known"_s));
        QVERIFY(english->isCorrect(u"castles"_s));

        QVERIFY(!english->isCorrect(u"helo"_s));
        QVERIFY(!english->isCorrect(u"magnificant"_s));
        QVERIFY(!english->isCorrect(u"recieve"_s));

        QVERIFY(english->suggestions(u"recieve"_s, 5).contains(u"receive"_s));
    }
};

QTEST_GUILESS_MAIN(SpellCheckersTest)

#include "SpellCheckersTest.moc"
