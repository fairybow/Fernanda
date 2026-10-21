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

#include <QFile>
#include <QIODevice>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QStandardPaths>
#include <QString>
#include <QTemporaryDir>
#include <QTest>
#include <QtGlobal>

#include <Coco/Path.h>

#include "core/AppConfig.h"
#include "core/AppDirs.h"
#include "core/Publication.h"

using namespace Qt::StringLiterals;

using Fernanda::AppConfig;

// The app's own settings file (core/AppConfig.h): the known-vaults list, whose
// records name folders by any spelling, the Common Vault's three states, and
// what survives a save and a load.
//
// Qt's test mode puts the settings file in a folder of its own, away from the
// real one, and each test starts with no file there. The vault folders are
// temporary folders, which the records only name.
//
// Qt Test runs every private slot as a test
class AppConfigTest : public QObject
{
    Q_OBJECT

private:
    [[nodiscard]] static Coco::Path filePath_()
    {
        return Fernanda::AppDirs::appData() /
               (PUB_APP_NAME_LOWER_QSTRING + u".json"_s);
    }

    // The same folder, spelled differently on every platform
    [[nodiscard]] static Coco::Path otherSpelling_(const Coco::Path& folder)
    {
        return Coco::Path(folder.toQString() + u"/sub/.."_s);
    }

    static void writeFile_(const QJsonObject& root)
    {
        QFile file(filePath_().toQString());
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(QJsonDocument(root).toJson());
    }

    static void removeFile_() { QFile::remove(filePath_().toQString()); }

private slots:
    void initTestCase() { QStandardPaths::setTestModeEnabled(true); }

    void init() { removeFile_(); }

    void cleanupTestCase() { removeFile_(); }

    // --- Known vaults -------------------------------------------------------

    // A known vault touched by another spelling is bumped, not listed twice,
    // and keeps the spelling it was recorded with
    void touchFindsARecordByAnySpelling()
    {
        QTemporaryDir vault{};
        Coco::Path root(vault.path());
        AppConfig config{};

        config.touchVault(root);
        config.touchVault(otherSpelling_(root));

        QCOMPARE(config.recentVaults(), Coco::PathList({ root }));
    }

    void everyVerbFindsARecordByAnySpelling()
    {
        QTemporaryDir vault{};
        QTemporaryDir elsewhere{};
        Coco::Path root(vault.path());
        Coco::Path moved(elsewhere.path());
        AppConfig config{};
        config.touchVault(root);

        config.setVaultOpen(otherSpelling_(root), true);
        QCOMPARE(config.openVaultPaths(), Coco::PathList({ root }));

        config.renameVault(otherSpelling_(root), moved);
        QCOMPARE(config.recentVaults(), Coco::PathList({ moved }));
        QCOMPARE(config.openVaultPaths(), Coco::PathList({ moved }));

        QVERIFY(config.forgetVault(otherSpelling_(moved)));
        QVERIFY(config.recentVaults().isEmpty());
    }

    // Renamed onto a folder that already has a record, the two are one: the
    // more recent timestamp, and open if either was
    void renameOntoAKnownFolderMerges()
    {
        QTemporaryDir vault{};
        QTemporaryDir elsewhere{};
        Coco::Path root(vault.path());
        Coco::Path moved(elsewhere.path());
        AppConfig config{};

        config.touchVault(moved);
        config.touchVault(root);
        config.setVaultOpen(moved, true);

        config.renameVault(root, otherSpelling_(moved));

        QCOMPARE(
            config.recentVaults(),
            Coco::PathList({ otherSpelling_(moved) }));
        QCOMPARE(
            config.openVaultPaths(),
            Coco::PathList({ otherSpelling_(moved) }));
    }

    // Where letter case is ignored, a folder spelled in another case is the
    // same vault. Elsewhere it's another folder, and another record
    void letterCaseCountsOnlyWhereTheFileSystemsDo()
    {
        QTemporaryDir vault{};
        Coco::Path root(vault.path());
        auto upper = root.parent() / Coco::Path(root.nameQString().toUpper());
        AppConfig config{};

        config.touchVault(root);
        config.touchVault(upper);

#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)

        QCOMPARE(config.recentVaults(), Coco::PathList({ root }));

#else

        QCOMPARE(config.recentVaults().size(), 2);

#endif
    }

    // A file can hold two records for one folder, written before records
    // were compared by key. Loaded, they're one: the newer one's spelling,
    // and open if either was. The next save writes the one
    void loadMergesRecordsForOneFolder()
    {
        QTemporaryDir vault{};
        Coco::Path root(vault.path());
        auto other = otherSpelling_(root);

        QJsonObject older{};
        older[u"path"_s] = root.toQString();
        older[u"lastUsedEpochMs"_s] = 100;
        older[u"open"_s] = true;

        QJsonObject newer{};
        newer[u"path"_s] = other.toQString();
        newer[u"lastUsedEpochMs"_s] = 200;

        QJsonObject file{};
        file[u"vaults"_s] = QJsonArray{ older, newer };
        writeFile_(file);

        AppConfig config{};
        config.load();

        QCOMPARE(config.recentVaults(), Coco::PathList({ other }));
        QCOMPARE(config.openVaultPaths(), Coco::PathList({ other }));

        QVERIFY(config.save());
        AppConfig reloaded{};
        reloaded.load();

        QCOMPARE(reloaded.recentVaults().size(), 1);
    }

    // --- The Common Vault ---------------------------------------------------

    // With no file, the first run hasn't set one up yet
    void theCommonVaultStartsNotSetUp()
    {
        AppConfig config{};
        config.load();

        QVERIFY(!config.isCommonVaultSetUp());
        QVERIFY(config.commonVaultPath().isEmpty());
    }

    // Set, it's saved and loaded as spelled, and it's a known vault
    void aSetCommonVaultIsSavedLoadedAndListed()
    {
        QTemporaryDir vault{};
        Coco::Path root(vault.path());
        AppConfig config{};

        config.setCommonVaultPath(root);
        QCOMPARE(config.recentVaults(), Coco::PathList({ root }));
        QVERIFY(config.save());

        AppConfig reloaded{};
        reloaded.load();

        QVERIFY(reloaded.isCommonVaultSetUp());
        QCOMPARE(reloaded.commonVaultPath(), root);
        QCOMPARE(reloaded.recentVaults(), Coco::PathList({ root }));
    }

    // Unset is a choice that's kept: set up, with no Common Vault. Its record
    // stays
    void anUnsetCommonVaultStaysUnset()
    {
        QTemporaryDir vault{};
        Coco::Path root(vault.path());
        AppConfig config{};
        config.setCommonVaultPath(root);

        config.setCommonVaultPath({});
        QVERIFY(config.save());

        AppConfig reloaded{};
        reloaded.load();

        QVERIFY(reloaded.isCommonVaultSetUp());
        QVERIFY(reloaded.commonVaultPath().isEmpty());
        QCOMPARE(reloaded.recentVaults(), Coco::PathList({ root }));
    }

    // A file naming a Common Vault it doesn't list (edited by hand) lists it
    void loadListsACommonVaultWithNoRecord()
    {
        QTemporaryDir vault{};
        Coco::Path root(vault.path());

        QJsonObject file{};
        file[u"commonVaultPath"_s] = root.toQString();
        writeFile_(file);

        AppConfig config{};
        config.load();

        QCOMPARE(config.recentVaults(), Coco::PathList({ root }));
    }

    // While it's set, the Common Vault can't be removed from the list
    void theCommonVaultCantBeForgotten()
    {
        QTemporaryDir vault{};
        Coco::Path root(vault.path());
        AppConfig config{};
        config.setCommonVaultPath(root);

        QVERIFY(!config.forgetVault(otherSpelling_(root)));
        QCOMPARE(config.recentVaults(), Coco::PathList({ root }));

        config.setCommonVaultPath({});
        QVERIFY(config.forgetVault(root));
        QVERIFY(config.recentVaults().isEmpty());
    }
};

QTEST_GUILESS_MAIN(AppConfigTest)

#include "AppConfigTest.moc"
