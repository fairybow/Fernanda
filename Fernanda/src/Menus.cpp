/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#include <QAction>
#include <QKeySequence>
#include <Qt>

#include "Application.h"
#include "Menus.h"
#include "Tr.h"
#include "Window.h"

namespace Fernanda::Menus::Internal {

QAction* makeAppQuitAction_(Window* window)
{
    if (!window) return nullptr;

    auto action = new QAction(Tr::Menus::fileQuit(), window);
    action->connect(
        action,
        &QAction::triggered,
        app(),
        &Application::tryQuit,
        Qt::QueuedConnection);
    action->setShortcut({ Qt::CTRL | Qt::Key_Q });
    action->setAutoRepeat(AutoRepeat::No);

    return action;
}

} // namespace Fernanda::Menus::Internal
