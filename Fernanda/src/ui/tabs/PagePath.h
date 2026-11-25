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

#pragma once

#include <QString>
#include <QVariant>
#include <QWidget>

namespace Fernanda::Ui {

// The path a pane's header shows for a page (TabHeader): where its file is,
// as the reader knows it, with folders separated by "/". For a file view it's
// the path from the root of the vault the file belongs to in its window, as
// the file tree shows it; for a diff, the path in the repository, as the Git
// page lists it. A new tab has none.
//
// Like the pin (PagePin) and the mark (PageIcon), it lives on the page widget,
// so it rides every move with the page, and the leaf reads it knowing nothing
// of page types. Whoever knows the path sets it: VaultWindow for a file view,
// when the view is made and whenever its file moves or the Common Vault
// changes, and a diff page for itself. A change under a page that's showing is
// drawn when its tree is asked (TabPaneTree::refreshPageHeader)

inline constexpr auto PAGE_PATH_PROPERTY = "fernanda_pagePath";

// Empty when the page has none, and for no page
[[nodiscard]] inline QString pagePath(QWidget* page)
{
    return page ? page->property(PAGE_PATH_PROPERTY).toString() : QString{};
}

inline void setPagePath(QWidget* page, const QString& path)
{
    if (page) {
        page->setProperty(PAGE_PATH_PROPERTY, path);
    }
}

} // namespace Fernanda::Ui
