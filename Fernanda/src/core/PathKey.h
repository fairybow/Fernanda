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

#include <cstddef>

#include <QChar>
#include <QHashFunctions>
#include <QString>
#include <QtGlobal>

#include <Coco/Path.h>

namespace Fernanda {

// Identifies a file or folder by its absolute path, as the key for anything
// that must exist once per file. PathKey::of is the only way to make one.
//
// Made from the path alone, without asking the file system, so a file that
// doesn't exist yet has the same key it will have once it does. "." and ".."
// are removed, separators become single forward slashes, and a trailing one
// is dropped, so every spelling of one path is one key. Comparison ignores
// letter case on Windows and macOS, whose file systems ignore it by default,
// and respects it elsewhere; the path keeps the case it was written in, for
// display and for file operations.
//
// Symlinks are not followed: a file reached through a symlinked folder has a
// different key from the same file reached directly. Vault roots are resolved
// through symlinks before keys are made from them (App does it), so the same
// folder opened two ways is one vault. See docs/Architecture.md, "Identity"
class PathKey
{
public:
    PathKey() = default;

    // Empty when `absolute` is empty or relative: an app has no meaningful
    // folder to resolve a relative path against
    [[nodiscard]] static PathKey of(const Coco::Path& absolute)
    {
        if (absolute.isEmpty() || !absolute.toStd().is_absolute()) {
            return {};
        }

        return PathKey(absolute.lexicallyNormal().prettyQString());
    }

    [[nodiscard]] bool isEmpty() const noexcept { return key_.isEmpty(); }
    [[nodiscard]] Coco::Path path() const { return path_; }

    bool operator==(const PathKey& other) const noexcept
    {
        return key_ == other.key_;
    }

    // True if this is `base` itself or something inside it
    [[nodiscard]] bool isAtOrUnder(const PathKey& base) const
    {
        return (!isEmpty() && *this == base) || isUnder(base);
    }

    // True if this is inside `base`, and is not `base` itself
    [[nodiscard]] bool isUnder(const PathKey& base) const
    {
        if (isEmpty() || base.isEmpty()) {
            return false;
        }

        auto prefix = prefixOf_(base);
        return key_.size() > prefix.size() && key_.startsWith(prefix);
    }

    // The path from `base` down to this, in this path's letter case. Empty
    // when this isn't inside `base`
    [[nodiscard]] Coco::Path relativeTo(const PathKey& base) const
    {
        if (!isUnder(base)) {
            return {};
        }

        return path_.prettyQString().mid(prefixOf_(base).size());
    }

    // Where this is once `oldBase` becomes `newBase`, as when a folder is
    // renamed or moved. Empty when this isn't `oldBase` or inside it
    [[nodiscard]] PathKey
    rebased(const PathKey& oldBase, const PathKey& newBase) const
    {
        if (!isEmpty() && *this == oldBase) {
            return newBase;
        }

        if (!isUnder(oldBase) || newBase.isEmpty()) {
            return {};
        }

        return PathKey((newBase.path_ / relativeTo(oldBase)).prettyQString());
    }

    friend std::size_t qHash(const PathKey& key, std::size_t seed = 0) noexcept
    {
        return qHash(key.key_, seed);
    }

private:
    Coco::Path path_{};
    QString key_{}; // what equality, hashing, and containment compare

    // pretty is a normal path's Coco::Path::prettyQString: forward slashes,
    // and no trailing separator except on a root
    explicit PathKey(const QString& pretty)
        : path_(pretty)
        , key_(keyOf_(pretty))
    {
    }

    // Folded one UTF-16 unit at a time, so the key is exactly as long as the
    // path's string and a position in one is the same position in the other
    // (relativeTo depends on that)
    [[nodiscard]] static QString keyOf_(const QString& pretty)
    {
        auto key = pretty;

#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)

        for (auto& ch : key) {
            ch = ch.toCaseFolded();
        }

#endif

        return key;
    }

    // What every key inside `base` starts with
    [[nodiscard]] static QString prefixOf_(const PathKey& base)
    {
        return base.key_.endsWith(u'/') ? base.key_ : base.key_ + u'/';
    }
};

} // namespace Fernanda
