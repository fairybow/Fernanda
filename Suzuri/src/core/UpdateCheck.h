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

#include <optional>

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QString>
#include <QStringList>
#include <QStringView>
#include <QUrl>
#include <QtMinMax>
#include <QtTypes>

// Whether a newer Suzuri has been released, decided from GitHub's list of
// releases (the JSON array the releases endpoint returns). Free functions with
// no network, so the decision is testable on its own; Updater fetches the list
// and asks.
//
// Versions are compared by Semantic Versioning's precedence rules, not as
// strings and not by the order GitHub lists releases in (newest created
// first, which a fix to an older line breaks). A release's version is its
// tag, which release.yml requires to be "v" plus VERSION_FULL_STRING, so the
// tag alone says whether a release is a prerelease
namespace Fernanda::UpdateCheck {

// A release newer than the running version: its version without the tag's
// "v", and the release's own page
struct Release
{
    QString version{};
    QUrl pageUrl{};
};

namespace Internal {

struct Version_
{
    qulonglong major = 0;
    qulonglong minor = 0;
    qulonglong patch = 0;
    QStringList prerelease{};
};

[[nodiscard]] inline bool isNumber_(QStringView text)
{
    if (text.isEmpty()) {
        return false;
    }

    for (auto c : text) {
        if (c < u'0' || c > u'9') {
            return false;
        }
    }

    return true;
}

// A prerelease identifier is ASCII letters, digits, and hyphens
[[nodiscard]] inline bool isIdentifier_(QStringView text)
{
    if (text.isEmpty()) {
        return false;
    }

    for (auto c : text) {
        auto ok = (c >= u'0' && c <= u'9') || (c >= u'a' && c <= u'z') ||
                  (c >= u'A' && c <= u'Z') || c == u'-';
        if (!ok) {
            return false;
        }
    }

    return true;
}

// MAJOR.MINOR.PATCH, then an optional -PRERELEASE of dot-separated
// identifiers. A leading "v" (a tag) is allowed, and build metadata after a
// "+" is dropped, since it takes no part in precedence
[[nodiscard]] inline std::optional<Version_> parse_(QStringView text)
{
    if (text.startsWith(u'v') || text.startsWith(u'V')) {
        text = text.sliced(1);
    }

    if (auto plus = text.indexOf(u'+'); plus >= 0) {
        text = text.first(plus);
    }

    QStringView core = text;
    QStringView prerelease{};
    auto has_prerelease = false;

    if (auto dash = text.indexOf(u'-'); dash >= 0) {
        core = text.first(dash);
        prerelease = text.sliced(dash + 1);
        has_prerelease = true;
    }

    auto numbers = core.split(u'.');
    if (numbers.size() != 3) {
        return std::nullopt;
    }

    Version_ version{};
    qulonglong* parts[] = { &version.major, &version.minor, &version.patch };

    for (auto i = 0; i < 3; ++i) {
        if (!isNumber_(numbers[i])) {
            return std::nullopt;
        }

        auto ok = false;
        *parts[i] = numbers[i].toULongLong(&ok);
        if (!ok) {
            return std::nullopt;
        }
    }

    if (has_prerelease) {
        for (auto identifier : prerelease.split(u'.')) {
            if (!isIdentifier_(identifier)) {
                return std::nullopt;
            }

            version.prerelease << identifier.toString();
        }
    }

    return version;
}

// Numeric identifiers compare as numbers and rank below alphanumeric ones,
// which compare in ASCII order
[[nodiscard]] inline int compareIdentifiers_(const QString& a, const QString& b)
{
    auto a_number = isNumber_(a);
    auto b_number = isNumber_(b);

    if (a_number && b_number) {
        auto a_value = a.toULongLong();
        auto b_value = b.toULongLong();
        return a_value < b_value ? -1 : a_value > b_value ? 1 : 0;
    }

    if (a_number != b_number) {
        return a_number ? -1 : 1;
    }

    auto result = QString::compare(a, b, Qt::CaseSensitive);
    return result < 0 ? -1 : result > 0 ? 1 : 0;
}

[[nodiscard]] inline int compare_(const Version_& a, const Version_& b)
{
    const qulonglong a_core[] = { a.major, a.minor, a.patch };
    const qulonglong b_core[] = { b.major, b.minor, b.patch };

    for (auto i = 0; i < 3; ++i) {
        if (a_core[i] != b_core[i]) {
            return a_core[i] < b_core[i] ? -1 : 1;
        }
    }

    // A prerelease ranks below the release it leads up to
    if (a.prerelease.isEmpty() || b.prerelease.isEmpty()) {
        if (a.prerelease.isEmpty() == b.prerelease.isEmpty()) {
            return 0;
        }

        return a.prerelease.isEmpty() ? 1 : -1;
    }

    // Identifier by identifier; where one runs out first, it ranks lower
    auto shared = qMin(a.prerelease.size(), b.prerelease.size());
    for (auto i = 0; i < shared; ++i) {
        auto result = compareIdentifiers_(a.prerelease[i], b.prerelease[i]);
        if (result != 0) {
            return result;
        }
    }

    if (a.prerelease.size() == b.prerelease.size()) {
        return 0;
    }

    return a.prerelease.size() < b.prerelease.size() ? -1 : 1;
}

} // namespace Internal

// Negative when a is older than b, zero when they're the same version,
// positive when a is newer. Nothing when either isn't a version
[[nodiscard]] inline std::optional<int>
compareVersions(const QString& a, const QString& b)
{
    auto a_version = Internal::parse_(a);
    auto b_version = Internal::parse_(b);

    if (!a_version || !b_version) {
        return std::nullopt;
    }

    return Internal::compare_(*a_version, *b_version);
}

// Whether a version has a prerelease part ("0.6.0-beta.0"). False for
// anything that isn't a version
[[nodiscard]] inline bool isPrerelease(const QString& version)
{
    auto parsed = Internal::parse_(version);
    return parsed && !parsed->prerelease.isEmpty();
}

// The newest release in releases that is newer than currentVersion, or
// nothing. A prerelease counts only when includePrereleases is true. Drafts,
// and releases whose tag isn't a version, are passed over
[[nodiscard]] inline std::optional<Release> availableUpdate(
    const QJsonArray& releases,
    const QString& currentVersion,
    bool includePrereleases)
{
    auto current = Internal::parse_(currentVersion);
    if (!current) {
        return std::nullopt;
    }

    std::optional<Internal::Version_> newest{};
    Release found{};

    for (const auto& value : releases) {
        auto release = value.toObject();

        if (release.value(u"draft").toBool()) {
            continue;
        }

        auto tag = release.value(u"tag_name").toString();
        auto version = Internal::parse_(tag);

        if (!version) {
            continue;
        }

        if (!version->prerelease.isEmpty() && !includePrereleases) {
            continue;
        }

        if (Internal::compare_(*version, *current) <= 0) {
            continue;
        }

        if (newest && Internal::compare_(*version, *newest) <= 0) {
            continue;
        }

        newest = version;
        found.version =
            tag.startsWith(u'v') || tag.startsWith(u'V') ? tag.sliced(1) : tag;
        found.pageUrl = QUrl(release.value(u"html_url").toString());
    }

    if (!newest) {
        return std::nullopt;
    }

    return found;
}

} // namespace Fernanda::UpdateCheck
