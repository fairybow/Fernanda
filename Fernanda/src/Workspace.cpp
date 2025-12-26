/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#include "Workspace.h"
#include "Application.h"
#include "MenuBuilder.h"
#include "MenuShortcuts.h"
#include "Tr.h"

namespace Fernanda {

void Workspace::addQuitAction(MenuBuilder& builder)
{
    builder.action(Tr::Menus::fileQuit())
        .slot(app(), &Application::tryQuit, Qt::QueuedConnection)
        .shortcut(MenuShortcuts::QUIT);
}

} // namespace Fernanda
