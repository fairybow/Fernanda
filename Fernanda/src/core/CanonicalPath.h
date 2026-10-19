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

// Identifies a file or folder by where it is on disk. Every spelling of one
// place (through a symlink, with "." or "..", or in another letter case where
// the file system ignores case) gives an equal value, so it can key anything
// that must exist once per file. CanonicalPath::of is the only way to make one.
//
// A path that doesn't exist yet (a file about to be created, a rename target)
// resolves through its nearest existing folder, with the rest kept as written.
// That is the path the file system reports once the file exists, so a value
// made early stays equal to one made after.
//
// The path keeps the letter case it resolved to, for display and for file
// operations. Comparison ignores case on Windows and macOS, whose file systems
// ignore it by default, and respects it elsewhere. Resolving alone can't settle
// case: Qt reports a path in the case it was written, not the case on disk
class CanonicalPath
{
public:
    CanonicalPath() = default;

    // Empty when `absolute` is empty or relative (an app has no meaningful
    // folder to resolve it against), or when not even its root exists
    [[nodiscard]] static CanonicalPath of(const Coco::Path& absolute)
    {
        if (absolute.isEmpty() || !absolute.toStd().is_absolute()) {
            return {};
        }

        auto existing = absolute.lexicallyNormal();

        // A trailing separator names no last component, so it would stall the
        // walk below. A root ("/", "C:/") is its own parent and keeps its own
        if (existing.name().isEmpty()) {
            existing = existing.parent();
        }

        Coco::Path rest{};

        while (true) {
            if (auto resolved = existing.canonical(); !resolved.isEmpty()) {
                return CanonicalPath(
                    rest.isEmpty() ? resolved : resolved / rest);
            }

            auto up = existing.parent();

            if (up == existing) { // a root that doesn't exist
                return {};
            }

            rest = rest.isEmpty() ? existing.name() : existing.name() / rest;
            existing = up;
        }
    }

    [[nodiscard]] bool isEmpty() const noexcept { return key_.isEmpty(); }
    [[nodiscard]] Coco::Path path() const { return path_; }

    bool operator==(const CanonicalPath& other) const noexcept
    {
        return key_ == other.key_;
    }

    // True if this is `base` itself or something inside it
    [[nodiscard]] bool isAtOrUnder(const CanonicalPath& base) const
    {
        return (!isEmpty() && *this == base) || isUnder(base);
    }

    // True if this is inside `base`, and is not `base` itself
    [[nodiscard]] bool isUnder(const CanonicalPath& base) const
    {
        if (isEmpty() || base.isEmpty()) {
            return false;
        }

        auto prefix = prefixOf_(base);
        return key_.size() > prefix.size() && key_.startsWith(prefix);
    }

    // The path from `base` down to this, in this path's letter case. Empty
    // when this isn't inside `base`
    [[nodiscard]] Coco::Path relativeTo(const CanonicalPath& base) const
    {
        if (!isUnder(base)) {
            return {};
        }

        return path_.prettyQString().mid(prefixOf_(base).size());
    }

    // Where this is once `oldBase` becomes `newBase`, as when a folder is
    // renamed or moved. Works from the paths alone, since the folder's
    // contents are wherever the rename put them. Empty when this isn't
    // `oldBase` or inside it
    [[nodiscard]] CanonicalPath
    rebased(const CanonicalPath& oldBase, const CanonicalPath& newBase) const
    {
        if (!isEmpty() && *this == oldBase) {
            return newBase;
        }

        if (!isUnder(oldBase) || newBase.isEmpty()) {
            return {};
        }

        return CanonicalPath(newBase.path_ / relativeTo(oldBase));
    }

    friend std::size_t
    qHash(const CanonicalPath& path, std::size_t seed = 0) noexcept
    {
        return qHash(path.key_, seed);
    }

private:
    Coco::Path path_{};
    QString key_{}; // what equality, hashing, and containment compare

    explicit CanonicalPath(const Coco::Path& resolved)
        : path_(resolved)
        , key_(keyOf_(resolved))
    {
    }

    // Forward slashes, no trailing separator except on a root. Folded one
    // UTF-16 unit at a time, so the key is exactly as long as the path's
    // string and a position in one is the same position in the other
    // (relativeTo depends on that)
    [[nodiscard]] static QString keyOf_(const Coco::Path& path)
    {
        auto key = path.prettyQString();

#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)

        for (auto& ch : key) {
            ch = ch.toCaseFolded();
        }

#endif

        return key;
    }

    // What every key inside `base` starts with
    [[nodiscard]] static QString prefixOf_(const CanonicalPath& base)
    {
        return base.key_.endsWith(u'/') ? base.key_ : base.key_ + u'/';
    }
};

} // namespace Fernanda
