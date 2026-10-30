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

#pragma once

#include <algorithm>
#include <optional>

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QSettings>
#include <QString>

#include <Coco/Path.h>
#include <Coco/Time.h>

#include "core/AppDirs.h"
#include "core/JsonIo.h"
#include "core/PathKey.h"
#include "core/Publication.h"
#include "core/VaultOrder.h"

// App-level configuration — the machine-local fernanda.json in appData(),
// mirroring Obsidian's obsidian.json. Owned by App as a plain by-value member:
// "only one exists" because App is one, not because it's a singleton. Nothing
// here ports to another machine.
//
// A plain value type, not a QObject. Most getters fall back to a default when
// their key is unset, so a caller never has to know whether the value came
// from disk; the Common Vault has three states instead (commonVaultPath). The
// vault registry is mutated through intent verbs (touchVault / setVaultOpen /
// forgetVault / renameVault); every mutator touches memory only, and App calls
// save() at the commit points — IO stays explicit rather than hiding inside a
// mutator.
//
// The vaults registry (path is identity, no id) is one array of { path,
// lastUsedEpochMs, open } records. Paths are compared as PathKeys, so every
// spelling of one folder finds its one record, which keeps the spelling it was
// recorded or renamed with. The two data fields carry distinct jobs:
// lastUsedEpochMs is a recency stamp (bumped on open and on activation) that
// orders the recents list and, on restore, the stacking order — nothing more;
// open marks a vault as part of the session to restore, maintained live as
// windows open and close (App), so a vault is reopened next launch iff it was
// up at the last exit. Reopen keys entirely off open — there's no timestamp
// fallback — so an empty open set means "show the picker", not "guess the
// newest vault". Matching Obsidian, open is written only when true.
namespace Fernanda {

using namespace Qt::StringLiterals;

class AppConfig
{
public:
    AppConfig() = default;

    // The Common Vault: one of the known vaults, whose files every window
    // shows beside its own. Empty when there is none: before the first run
    // sets one up, or once the user unsets it or its folder goes
    [[nodiscard]] Coco::Path commonVaultPath() const
    {
        return commonVaultPath_.value_or(Coco::Path{});
    }

    // False only until the first run sets up the Common Vault
    // (App::initCommonVault_). After that the user's choice stands, even a
    // choice of none
    [[nodiscard]] bool isCommonVaultSetUp() const noexcept
    {
        return commonVaultPath_.has_value();
    }

    // Set the Common Vault, or unset it with an empty path; either way it's set
    // up from then on. A Common Vault is always a known vault, so a path the
    // list lacks is added to it, as never used
    void setCommonVaultPath(const Coco::Path& path)
    {
        commonVaultPath_ = path;
        listCommonVault_();
    }

    // The starting directory for the create/open flow — the last place a vault
    // was created from or opened at. Unset ⇒ AppDirs::defaultDocs()
    [[nodiscard]] Coco::Path newVaultMruDir() const
    {
        return newVaultMruDir_.isEmpty() ? AppDirs::defaultDocs()
                                         : newVaultMruDir_;
    }

    void setNewVaultMruDir(const Coco::Path& dir) { newVaultMruDir_ = dir; }

    // Whether Fernanda checks for updates on its own (Updater). Absent from
    // fernanda.json, it follows the install (installChecksForUpdates_): on, as
    // Obsidian's automatic updates are, unless the installer's box was
    // unchecked
    [[nodiscard]] bool checksForUpdates() const noexcept
    {
        return checksForUpdates_;
    }

    void setChecksForUpdates(bool on) noexcept { checksForUpdates_ = on; }

    // The version whose update notice was dismissed (Updater). Empty when
    // none has been
    [[nodiscard]] QString dismissedUpdate() const { return dismissedUpdate_; }

    void setDismissedUpdate(const QString& version)
    {
        dismissedUpdate_ = version;
    }

    // The version whose beta warning was muted (BetaAlert). Empty when none
    // has been
    [[nodiscard]] QString betaAlertMutedFor() const
    {
        return betaAlertMutedFor_;
    }

    void setBetaAlertMutedFor(const QString& version)
    {
        betaAlertMutedFor_ = version;
    }

    // The interface language (Translations): a language code, or empty to
    // follow the system
    [[nodiscard]] QString language() const { return language_; }

    void setLanguage(const QString& language) { language_ = language; }

    // The order of Manage Vaults' list. Recent unless another was chosen
    [[nodiscard]] VaultOrder vaultOrder() const noexcept { return vaultOrder_; }

    void setVaultOrder(VaultOrder order) noexcept { vaultOrder_ = order; }

    // The vaults list, most-recently-used first — the backing store for the
    // surfaces ManageVaults' recents column and each VaultSwitcher render.
    // Derived: the registry sorted by lastUsedEpochMs descending. Like
    // Obsidian's vault list this is every known vault, not a last-N window — no
    // cap; pruning is remove-from-list only (forgetVault). An empty registry
    // yields an empty list, the correct first-run value
    [[nodiscard]] Coco::PathList recentVaults() const
    {
        Coco::PathList paths{};
        paths.reserve(vaults_.size());

        for (const auto& record : sortedByLastUsedDesc_()) {
            paths << record.path;
        }

        return paths;
    }

    // The vaults to reopen at launch: those marked open at the last exit,
    // oldest-used first. That ordering IS the restore order — reopening in it
    // activates the most-recent vault last, so it lands frontmost and focused,
    // and the re-stamped timestamps keep their relative order. sortedByLastUsed
    // is newest-first, so this walks it in reverse. The caller filters to
    // still-existing folders and shows the picker when none qualify — there's
    // no timestamp fallback
    [[nodiscard]] Coco::PathList openVaultPaths() const
    {
        const auto sorted = sortedByLastUsedDesc_();
        Coco::PathList paths{};

        for (auto it = sorted.crbegin(); it != sorted.crend(); ++it) {
            if (it->open) {
                paths << it->path;
            }
        }

        return paths;
    }

    // Promote a vault to most-recently-used by stamping its lastUsedEpochMs
    // with now — an upsert: a first-time vault is appended (open false, the
    // caller sets it), a known one is bumped and keeps the spelling it was
    // recorded with. Used by both the open convergence point and window
    // activation, so recency tracks last USE, not merely last open. Memory
    // only; the caller saves
    void touchVault(const Coco::Path& root)
    {
        auto last_used = Coco::Time::epoch();

        if (auto* record = recordOf_(root)) {
            record->lastUsedEpochMs = last_used;
            return;
        }

        vaults_ << VaultRecord_{ root, last_used, false };
    }

    // Set one vault's open flag, the live session-restore signal. App sets it
    // true at the open convergence point and false when the window closes while
    // another vault is still open — leaving the last one set as the reopen
    // anchor. No-op for an unknown path; on the open path touchVault has
    // already upserted the record. Memory only; the caller saves
    void setVaultOpen(const Coco::Path& root, bool open)
    {
        if (auto* record = recordOf_(root)) {
            record->open = open;
        }
    }

    // Remove-from-list: drop one record, folder untouched. Returns whether
    // anything was removed, so App can skip a spurious save + signal when the
    // path wasn't listed. The Common Vault's record is refused: it stays
    // listed while it's set
    bool forgetVault(const Coco::Path& root)
    {
        if (auto common = commonVaultPath();
            !common.isEmpty() && sameVault_(common, root)) {
            return false;
        }

        return vaults_.removeIf([&root](const VaultRecord_& record) {
            return sameVault_(record.path, root);
        });
    }

    // Rename in place: swap one record's path, keeping its timestamp and open
    // flag. The vault keeps its recents position (a rename isn't a use). The
    // one way a record's spelling changes, so a change of letter case alone
    // is a rename too. No-op if the old path isn't listed.
    //
    // A record already naming the new folder (one listed as missing there, or
    // the Common Vault's, listed when it was set there) is merged in, as load
    // merges: the more recent timestamp, and open if either was. So no two
    // records ever name one folder
    void renameVault(const Coco::Path& oldRoot, const Coco::Path& newRoot)
    {
        auto* record = recordOf_(oldRoot);
        if (!record) {
            return;
        }

        auto* existing = recordOf_(newRoot);

        if (existing && existing != record) {
            record->lastUsedEpochMs =
                std::max(record->lastUsedEpochMs, existing->lastUsedEpochMs);
            record->open = record->open || existing->open;
            record->path = newRoot;

            vaults_.removeIf([existing](const VaultRecord_& other) {
                return &other == existing;
            });

            return;
        }

        record->path = newRoot;
    }

    // Read fernanda.json into memory. A missing file (first run) or an absent
    // "vaults" key leaves the registry empty and every path value unset, so
    // every getter returns its default. JsonIo/Io have already logged a
    // genuinely corrupt file; a missing one is silent.
    //
    // The Common Vault's key is read for three states: absent (not set up),
    // null (unset), or its path. A path with no record (a hand-edited file) is
    // listed
    void load()
    {
        auto root = JsonIo::read(filePath_());

        if (root.contains(COMMON_VAULT_PATH_KEY_)) {
            commonVaultPath_ =
                Coco::Path(root.value(COMMON_VAULT_PATH_KEY_).toString());
        } else {
            commonVaultPath_.reset();
        }

        assignPath_(root, u"newVaultMruDir"_s, newVaultMruDir_);

        installChecksForUpdates_ = readInstallChecksForUpdates_();
        checksForUpdates_ =
            root.value(u"checkForUpdates"_s).toBool(installChecksForUpdates_);
        dismissedUpdate_ = root.value(u"dismissedUpdate"_s).toString();
        betaAlertMutedFor_ = root.value(u"betaAlertMutedFor"_s).toString();
        language_ = root.value(u"language"_s).toString();
        vaultOrder_ = vaultOrderOf_(root.value(VAULT_ORDER_KEY_).toString());

        vaults_ = readVaults_(root.value(u"vaults"_s).toArray());
        listCommonVault_();
    }

    // Serialize the set keys and write atomically. Only keys with a value are
    // written, so "unset ⇒ absent ⇒ default" round-trips cleanly and the file
    // stays minimal. The Common Vault is the exception: once set up it's
    // always written, as null when unset, since its absence means the first
    // run hasn't set one up. Returns JsonIo/Io's success; App may ignore it
    // for non-critical values
    bool save() const
    {
        QJsonObject root{};

        if (commonVaultPath_) {
            root[COMMON_VAULT_PATH_KEY_] =
                commonVaultPath_->isEmpty()
                    ? QJsonValue(QJsonValue::Null)
                    : QJsonValue(commonVaultPath_->prettyQString());
        }

        if (!newVaultMruDir_.isEmpty()) {
            root[u"newVaultMruDir"_s] = newVaultMruDir_.prettyQString();
        }

        if (checksForUpdates_ != installChecksForUpdates_) {
            root[u"checkForUpdates"_s] = checksForUpdates_;
        }

        if (!dismissedUpdate_.isEmpty()) {
            root[u"dismissedUpdate"_s] = dismissedUpdate_;
        }

        if (!betaAlertMutedFor_.isEmpty()) {
            root[u"betaAlertMutedFor"_s] = betaAlertMutedFor_;
        }

        if (!language_.isEmpty()) {
            root[u"language"_s] = language_;
        }

        if (vaultOrder_ != VaultOrder::Recent) {
            root[VAULT_ORDER_KEY_] = nameOf_(vaultOrder_);
        }

        if (!vaults_.isEmpty()) {
            root[u"vaults"_s] = writeVaults_();
        }

        return JsonIo::write(root, filePath_(), Io::CreateDirs::Yes);
    }

private:
    static inline const QString COMMON_VAULT_PATH_KEY_ = u"commonVaultPath"_s;

    // The vault order is written by name, and only when it isn't Recent
    static inline const QString VAULT_ORDER_KEY_ = u"vaultOrder"_s;
    static inline const QString NAME_ASCENDING_ = u"nameAscending"_s;
    static inline const QString NAME_DESCENDING_ = u"nameDescending"_s;

    // No value until set up; then an empty path while unset, else the path
    std::optional<Coco::Path> commonVaultPath_{};

    // Empty == unset; the getter substitutes the AppDirs default
    Coco::Path newVaultMruDir_{};

    // Written only when it differs from the install's default. An unwritten
    // value follows that default, including one a later install changes
    bool checksForUpdates_ = true;
    bool installChecksForUpdates_ = true;
    QString dismissedUpdate_{};
    QString betaAlertMutedFor_{};
    QString language_{};
    VaultOrder vaultOrder_ = VaultOrder::Recent;

    // One registry entry: path is identity, lastUsedEpochMs is a recency stamp
    // (ms since the Unix epoch, Coco::Time::epoch) that orders the list and the
    // restore stacking, and open marks a vault to reopen next launch. The
    // timestamp serializes cleanly as a JSON number and stays exact well past
    // any real value
    struct VaultRecord_
    {
        Coco::Path path{};
        qint64 lastUsedEpochMs = 0;
        bool open = false;
    };

    // The vault registry. Unordered here — reads sort on demand, and the key is
    // the path, so array position carries no meaning. No two records name the
    // same folder: every mutator matches by sameVault_, and load merges any
    // the file holds
    QList<VaultRecord_> vaults_{};

    // Whether a and b name the same folder: compared as PathKeys, so a trailing
    // separator, "." and "..", and (on Windows and macOS) letter case don't
    // split one vault into two records. A path that makes no key (a relative
    // one, from a hand-edited file) is compared as written
    [[nodiscard]] static bool
    sameVault_(const Coco::Path& a, const Coco::Path& b)
    {
        auto a_key = PathKey::of(a);

        if (a_key.isEmpty()) {
            return a == b;
        }

        return a_key == PathKey::of(b);
    }

    [[nodiscard]] VaultRecord_* recordOf_(const Coco::Path& root)
    {
        for (auto& record : vaults_) {
            if (sameVault_(record.path, root)) {
                return &record;
            }
        }

        return nullptr;
    }

    // A set Common Vault that has no record gets one: never used, so last in
    // the recents, and not open
    void listCommonVault_()
    {
        auto common = commonVaultPath();

        if (!common.isEmpty() && !recordOf_(common)) {
            vaults_ << VaultRecord_{ common, 0, false };
        }
    }

    // A name the file doesn't know (edited by hand) reads as Recent
    [[nodiscard]] static VaultOrder vaultOrderOf_(const QString& name)
    {
        if (name == NAME_ASCENDING_) {
            return VaultOrder::NameAscending;
        }

        if (name == NAME_DESCENDING_) {
            return VaultOrder::NameDescending;
        }

        return VaultOrder::Recent;
    }

    [[nodiscard]] static QString nameOf_(VaultOrder order)
    {
        switch (order) {
        case VaultOrder::NameAscending:
            return NAME_ASCENDING_;
        case VaultOrder::NameDescending:
            return NAME_DESCENDING_;
        case VaultOrder::Recent:
            break;
        }

        return {};
    }

    static Coco::Path filePath_()
    {
        return AppDirs::appData() / (PUB_APP_NAME_LOWER_QSTRING + u".json"_s);
    }

    // The Windows installer's "Check for updates automatically" box, for
    // everyone who uses this install. The installer (WindowsInstaller.iss)
    // records it beside the executable, in install.ini, only when unchecked.
    // No file or no key, as on macOS, Linux, and a build run in place, is on
    static bool readInstallChecksForUpdates_()
    {
        auto path = Coco::Path::AppDir("install.ini");
        QSettings ini(path.toQString(), QSettings::IniFormat);

        return ini.value(u"Updates/CheckAutomatically"_s, true).toBool();
    }

    // Only overwrite the target when the key is present and non-empty, so a
    // partial or hand-trimmed file leaves the rest at their defaults
    static void
    assignPath_(const QJsonObject& root, const QString& key, Coco::Path& target)
    {
        auto value = root.value(key).toString();

        if (!value.isEmpty()) {
            target = Coco::Path(value);
        }
    }

    // { path, lastUsedEpochMs, open }[] ⇒ records. Blank-path entries are
    // skipped defensively (hand-trimmed file); an absent timestamp falls to 0
    // and an absent open to false — the Obsidian-style omitted-when-false form.
    //
    // Records naming one folder (sameVault_) are merged, as a file written
    // before paths were compared as keys can hold: the most recently used
    // one's spelling and timestamp, and open if any of them was
    static QList<VaultRecord_> readVaults_(const QJsonArray& array)
    {
        QList<VaultRecord_> records{};
        records.reserve(array.size());

        for (const auto& value : array) {
            auto object = value.toObject();
            auto path = object.value(u"path"_s).toString();

            if (path.isEmpty()) {
                continue;
            }

            VaultRecord_ read{ Coco::Path(path),
                               object.value(u"lastUsedEpochMs"_s).toInteger(),
                               object.value(u"open"_s).toBool() };

            auto same = std::find_if(
                records.begin(),
                records.end(),
                [&read](const VaultRecord_& record) {
                    return sameVault_(record.path, read.path);
                });

            if (same == records.end()) {
                records << read;
                continue;
            }

            if (read.lastUsedEpochMs > same->lastUsedEpochMs) {
                same->path = read.path;
                same->lastUsedEpochMs = read.lastUsedEpochMs;
            }

            same->open = same->open || read.open;
        }

        return records;
    }

    // Written newest-first so the file reads in the same order the surfaces
    // show it — cosmetic, since load() keys by path and sorts by timestamp
    // regardless. open is written only when true (matching Obsidian), so a
    // closed vault carries just path + lastUsedEpochMs
    QJsonArray writeVaults_() const
    {
        QJsonArray array{};

        for (const auto& record : sortedByLastUsedDesc_()) {
            QJsonObject object{};
            object[u"path"_s] = record.path.prettyQString();
            object[u"lastUsedEpochMs"_s] = record.lastUsedEpochMs;

            if (record.open) {
                object[u"open"_s] = true;
            }

            array.append(object);
        }

        return array;
    }

    // The registry copied and sorted most-recently-used first — the one place
    // the recency ordering lives, shared by recentVaults(), openVaultPaths(),
    // and writeVaults_(). Stable, so equal timestamps (a rare same-millisecond
    // touch) keep their prior order
    QList<VaultRecord_> sortedByLastUsedDesc_() const
    {
        auto sorted = vaults_;

        std::stable_sort(
            sorted.begin(),
            sorted.end(),
            [](const VaultRecord_& a, const VaultRecord_& b) {
                return a.lastUsedEpochMs > b.lastUsedEpochMs;
            });

        return sorted;
    }
};

} // namespace Fernanda
