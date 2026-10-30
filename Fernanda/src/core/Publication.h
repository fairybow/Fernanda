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

// clang-format off

#define PUB_AUTHOR_STRING                       "fairybow"
#define PUB_APP_NAME_STRING                     "Fernanda"
#define PUB_APP_NAME_LOWER_STRING               "fernanda"
#define PUB_RELEASE_NAME_STRING                 "Bashō"
#define PUB_COPYRIGHT_STRING                    "Copyright (C) 2026 fairybow"
#define PUB_DOMAIN_STRING                       "https://github.com/fairybow/Fernanda"

// NB: Reverse-DNS app ID (macOS bundle ID, .desktop name, Wayland app_id).
// Intentionally a literal, not derived from PUB_APP_NAME_STRING: it must never
// change, or user state the OS keys to it is orphaned
#define PUB_APP_ID_STRING                       "io.github.fairybow.Fernanda"

#ifndef RC_INVOKED

#   include <QString>

#   define PUB_AUTHOR_QSTRING                   QStringLiteral(PUB_AUTHOR_STRING)
#   define PUB_APP_NAME_QSTRING                 QStringLiteral(PUB_APP_NAME_STRING)
#   define PUB_APP_NAME_LOWER_QSTRING           QStringLiteral(PUB_APP_NAME_LOWER_STRING)
#   define PUB_RELEASE_NAME_QSTRING             QStringLiteral(PUB_RELEASE_NAME_STRING)
#   define PUB_COPYRIGHT_QSTRING                QStringLiteral(PUB_COPYRIGHT_STRING)
#   define PUB_DOMAIN_QSTRING                   QStringLiteral(PUB_DOMAIN_STRING)
#   define PUB_APP_ID_QSTRING                   QStringLiteral(PUB_APP_ID_STRING)

#endif // RC_INVOKED

// clang-format on
