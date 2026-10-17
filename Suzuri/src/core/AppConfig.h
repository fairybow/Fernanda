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

#pragma once

#include <algorithm>

#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QString>

#include <Coco/Path.h>
#include <Coco/Time.h>

#include "core/AppDirs.h"
#include "core/JsonIo.h"
#include "core/Publication.h"

// App-level configuration — the machine-local fernanda.json in appData(),
// mirroring Obsidian's obsidian.json. Owned by App as a plain by-value member:
// "only one exists" because App is one, not because it's a singleton. Nothing
// here ports to another machine.
//
// A plain value type, not a QObject. Getters fall back to the AppDirs defaults
// when a key is unset, so a caller never has to know whether the value came
// from disk or the default. The vault registry is mutated through intent verbs
// (touchVault / setVaultOpen / forgetVault / renameVault); every mutator
// touches memory only, and App calls save() at the commit points — IO stays
// explicit rather than hiding inside a mutator.
//
// The vaults registry (path is identity, no id) is one array of { path,
// lastUsedEpochMs, open } records. The two data fields carry distinct jobs:
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

    // The relocatable Common Vault root. Unset ⇒ AppDirs::defaultCommonVault().
    // In-app relocation isn't offered; this is only the read side, which
    // resolves App::init's common-vault location before it constructs the
    // windowless common Vault
    [[nodiscard]] Coco::Path commonVaultPath() const
    {
        return commonVaultPath_.isEmpty() ? AppDirs::defaultCommonVault()
                                          : commonVaultPath_;
    }

    void setCommonVaultPath(const Coco::Path& path) { commonVaultPath_ = path; }

    // The starting directory for the create/open flow — the last place a vault
    // was created from or opened at. Unset ⇒ AppDirs::defaultDocs()
    [[nodiscard]] Coco::Path newVaultMruDir() const
    {
        return newVaultMruDir_.isEmpty() ? AppDirs::defaultDocs()
                                         : newVaultMruDir_;
    }

    void setNewVaultMruDir(const Coco::Path& dir) { newVaultMruDir_ = dir; }

    // Whether Suzuri checks for updates on its own (Updater). On unless
    // switched off, as Obsidian's automatic updates are
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

    // The interface language (Translations): a language code, or empty to
    // follow the system
    [[nodiscard]] QString language() const { return language_; }

    void setLanguage(const QString& language) { language_ = language; }

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
    // caller sets it), a known one is bumped. Used by both the open convergence
    // point and window activation, so recency tracks last USE, not merely last
    // open. Memory only; the caller saves
    void touchVault(const Coco::Path& root)
    {
        auto last_used = Coco::Time::epoch();

        for (auto& record : vaults_) {
            if (record.path == root) {
                record.lastUsedEpochMs = last_used;
                return;
            }
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
        for (auto& record : vaults_) {
            if (record.path == root) {
                record.open = open;
                return;
            }
        }
    }

    // Remove-from-list: drop one record, folder untouched. Returns whether
    // anything was removed, so App can skip a spurious save + signal when the
    // path wasn't listed
    bool forgetVault(const Coco::Path& root)
    {
        return vaults_.removeIf([&root](const VaultRecord_& record) {
            return record.path == root;
        });
    }

    // Rename in place: swap one record's path, keeping its timestamp and open
    // flag. The vault keeps its recents position (a rename isn't a use). No-op
    // if the old path isn't listed
    void renameVault(const Coco::Path& oldRoot, const Coco::Path& newRoot)
    {
        for (auto& record : vaults_) {
            if (record.path == oldRoot) {
                record.path = newRoot;
                return;
            }
        }
    }

    // Read fernanda.json into memory. A missing file (first run) or an absent
    // "vaults" key leaves the registry empty and every path value unset, so
    // every getter returns its default. JsonIo/Io have already logged a
    // genuinely corrupt file; a missing one is silent
    void load()
    {
        auto root = JsonIo::read(filePath_());

        assignPath_(root, u"commonVaultPath"_s, commonVaultPath_);
        assignPath_(root, u"newVaultMruDir"_s, newVaultMruDir_);

        checksForUpdates_ = root.value(u"checkForUpdates"_s).toBool(true);
        dismissedUpdate_ = root.value(u"dismissedUpdate"_s).toString();
        language_ = root.value(u"language"_s).toString();

        vaults_ = readVaults_(root.value(u"vaults"_s).toArray());
    }

    // Serialize the set keys and write atomically. Only keys with a value are
    // written, so "unset ⇒ absent ⇒ default" round-trips cleanly and the file
    // stays minimal. Returns JsonIo/Io's success; App may ignore it for
    // non-critical values
    bool save() const
    {
        QJsonObject root{};

        if (!commonVaultPath_.isEmpty()) {
            root[u"commonVaultPath"_s] = commonVaultPath_.prettyQString();
        }

        if (!newVaultMruDir_.isEmpty()) {
            root[u"newVaultMruDir"_s] = newVaultMruDir_.prettyQString();
        }

        if (!checksForUpdates_) {
            root[u"checkForUpdates"_s] = false;
        }

        if (!dismissedUpdate_.isEmpty()) {
            root[u"dismissedUpdate"_s] = dismissedUpdate_;
        }

        if (!language_.isEmpty()) {
            root[u"language"_s] = language_;
        }

        if (!vaults_.isEmpty()) {
            root[u"vaults"_s] = writeVaults_();
        }

        return JsonIo::write(root, filePath_(), Io::CreateDirs::Yes);
    }

private:
    // Empty == unset; the getter substitutes the AppDirs default
    Coco::Path commonVaultPath_{};
    Coco::Path newVaultMruDir_{};

    // Written only when off, like a vault's open flag only when true
    bool checksForUpdates_ = true;
    QString dismissedUpdate_{};
    QString language_{};

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
    // the path, so array position carries no meaning
    QList<VaultRecord_> vaults_{};

    static Coco::Path filePath_()
    {
        return AppDirs::appData() / (PUB_APP_NAME_LOWER_QSTRING + u".json"_s);
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
    // and an absent open to false — the Obsidian-style omitted-when-false form
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

            records << VaultRecord_{
                Coco::Path(path),
                object.value(u"lastUsedEpochMs"_s).toInteger(),
                object.value(u"open"_s).toBool()
            };
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
