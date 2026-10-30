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

#include <optional>

#include <QString>
#include <QStringList>
#include <QUrl>

// What a remote's URL says about where it is, for opening a page on GitHub.
// Free functions with no process, so they're testable on their own.
//
// GitHub's repositories are reached in three forms, each read here:
// - https://github.com/owner/repo(.git), with or without a user before the
//   host
// - git@github.com:owner/repo(.git), git's short form for SSH
// - ssh://git@github.com/owner/repo(.git), with or without a port
//
// Only github.com counts: a GitHub Enterprise server, or any other host, has
// pages of its own shape, or none
namespace Fernanda::GitRemote {

using namespace Qt::StringLiterals;

namespace Internal {

// "owner/repo" from the part after the host, with any ".git" and slashes
// around it dropped. Nothing unless exactly two parts are left
[[nodiscard]] inline std::optional<QString> ownerRepoOf_(QString path)
{
    while (path.startsWith(u'/')) {
        path.remove(0, 1);
    }

    while (path.endsWith(u'/')) {
        path.chop(1);
    }

    if (path.endsWith(u".git"_s)) {
        path.chop(4);
    }

    auto parts = path.split(u'/');

    if (parts.size() != 2 || parts[0].isEmpty() || parts[1].isEmpty()) {
        return std::nullopt;
    }

    return path;
}

} // namespace Internal

// "owner/repo" when url is a github.com repository, in any of the forms above
[[nodiscard]] inline std::optional<QString> gitHubRepoOf(const QString& url)
{
    auto trimmed = url.trimmed();

    // git@github.com:owner/repo, which has no scheme
    if (!trimmed.contains(u"://"_s)) {
        auto at = trimmed.indexOf(u'@');
        auto colon = trimmed.indexOf(u':');

        if (colon < 0 || at > colon) {
            return std::nullopt;
        }

        auto host = trimmed.mid(at + 1, colon - at - 1);

        if (host.compare(u"github.com"_s, Qt::CaseInsensitive) != 0) {
            return std::nullopt;
        }

        return Internal::ownerRepoOf_(trimmed.mid(colon + 1));
    }

    QUrl parsed(trimmed);

    if (!parsed.isValid() ||
        parsed.host().compare(u"github.com"_s, Qt::CaseInsensitive) != 0) {
        return std::nullopt;
    }

    auto scheme = parsed.scheme().toLower();

    if (scheme != u"https"_s && scheme != u"http"_s && scheme != u"ssh"_s) {
        return std::nullopt;
    }

    return Internal::ownerRepoOf_(parsed.path());
}

// The page where GitHub offers to open a pull request from branch, into the
// repository's default branch. ownerRepo is from gitHubRepoOf. A branch's
// name may hold slashes, which stay as they are; anything else a URL can't
// carry is encoded
[[nodiscard]] inline QUrl
compareUrl(const QString& ownerRepo, const QString& branch)
{
    QUrl url{};
    url.setScheme(u"https"_s);
    url.setHost(u"github.com"_s);
    url.setPath(u'/' + ownerRepo + u"/compare/"_s + branch, QUrl::DecodedMode);
    url.setQuery(u"expand=1"_s);
    return url;
}

} // namespace Fernanda::GitRemote
