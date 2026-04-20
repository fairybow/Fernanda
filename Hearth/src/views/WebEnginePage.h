/*
 * Hearth — a plain-text-first workbench for creative writing
 * Copyright (C) 2025-2026 fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <QDesktopServices>
#include <QUrl>
#include <QWebEnginePage>

namespace Fernanda {

class WebEnginePage : public QWebEnginePage
{
    Q_OBJECT

public:
    using QWebEnginePage::QWebEnginePage;

protected:
    bool acceptNavigationRequest(
        const QUrl& url,
        NavigationType type,
        [[maybe_unused]] bool isMainFrame) override
    {
        // Let's not open links in the preview page...
        if (type == NavigationTypeLinkClicked) {
            QDesktopServices::openUrl(url);
            return false;
        }

        return true;
    }
};

} // namespace Fernanda
